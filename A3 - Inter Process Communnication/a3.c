#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define VARIANT 23475
#define LOGICAL_ALIGNMENT 2048
#define REQ_FIFO_NAME "REQ_PIPE_23475"
#define SHM_NAME "/UnRxv3"
#define RESP_FIFO_NAME "RESP_PIPE_23475"

void write_error(int fd);
void write_success(int fd);
void write_command(int fd, char length, char *command);
int is_valid_SF(unsigned char *mapped_sf, unsigned int file_size);

int main()
{
    unlink(RESP_FIFO_NAME);
    if (mkfifo(RESP_FIFO_NAME, 0600) < 0)
    {
        perror("ERROR\ncannot create the response pipe | cannot open the request pipe\n");
        return -1;
    }
    int req_fd = open(REQ_FIFO_NAME, O_RDONLY);
    if (req_fd < 0)
    {
        unlink(RESP_FIFO_NAME);
        perror("ERROR\ncannot create the response pipe | cannot open the request pipe\n");
        return -2;
    }
    int resp_fd = open(RESP_FIFO_NAME, O_WRONLY);
    if (resp_fd < 0)
    {
        unlink(RESP_FIFO_NAME);
        perror("ERROR\ncannot create the response pipe | cannot open the request pipe\n");
        return -2;
    }
    char len = 7;
    write_command(resp_fd, len, "CONNECT");
    printf("SUCCESS\n");

    int variant = VARIANT;
    char c_length = 0;
    int field_length;
    char *field, *filename;
    int shm_fd, file_fd;
    unsigned char *shm_ptr = NULL;
    unsigned char *mfile_ptr = NULL;
    unsigned int shm_size = 0;
    unsigned int mfile_size = 0;

    for (;;)
    {
        read(req_fd, &c_length, sizeof(char));
        field_length = (int)c_length;
        field = malloc(field_length + 1);
        field[field_length] = 0;
        read(req_fd, field, field_length);
        switch (field_length)
        {
        case 4:
            if (strcmp("EXIT", field) == 0)
            {
                free(field);
                close(file_fd);
                close(shm_fd);
                unlink(filename);
                munmap(shm_ptr, shm_size);
                munmap(mfile_ptr, mfile_size);
                shm_unlink(SHM_NAME);
                unlink(RESP_FIFO_NAME);
                return 0;
            }
            else if (strcmp("PING", field) == 0)
            {
                write(resp_fd, &c_length, 1);
                write(resp_fd, field, field_length);
                write(resp_fd, &c_length, 1);
                write(resp_fd, "PONG", 4);
                write(resp_fd, &variant, sizeof(int));
            }
            break;
        case 10:
            if (strcmp("CREATE_SHM", field) == 0)
            {
                int ok = 1;
                read(req_fd, &shm_size, sizeof(unsigned int));

                shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
                if (shm_fd > 0)
                {
                    ftruncate(shm_fd, shm_size);
                    shm_ptr = (unsigned char *)mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                    if (shm_ptr == (void *)-1)
                    {
                        close(shm_fd);
                        perror("mmap failed");
                        ok = 0;
                    }
                }
                else
                {
                    perror("bad file descriptor");
                    ok = 0;
                }

                write_command(resp_fd, c_length, field);

                if (ok == 1)
                {
                    write_success(resp_fd);
                }
                else
                {
                    write_error(resp_fd);
                }
            }
            break;
        case 12:
            if (strcmp("WRITE_TO_SHM", field) == 0)
            {
                unsigned int offset, value;
                read(req_fd, &offset, sizeof(unsigned int));
                read(req_fd, &value, sizeof(unsigned int));

                write_command(resp_fd, c_length, field);

                if (shm_ptr != NULL && offset > 0 && offset < (shm_size - 1) - sizeof(unsigned int))
                {
                    *((int *)(shm_ptr + offset)) = value;
                    write_success(resp_fd);
                }
                else
                {
                    write_error(resp_fd);
                }
            }
            break;
        case 8:
            if (strcmp("MAP_FILE", field) == 0)
            {
                char filename_len;

                read(req_fd, &filename_len, 1);
                filename = malloc(1 + (int)filename_len);
                filename[(int)filename_len] = 0;
                read(req_fd, filename, (int)filename_len);

                write_command(resp_fd, c_length, field);
                file_fd = open(filename, O_RDONLY);
                if (file_fd < 0)
                {
                    write_error(resp_fd);
                }
                else
                {
                    mfile_size = lseek(file_fd, 0, SEEK_END);
                    lseek(file_fd, 0, SEEK_SET);

                    mfile_ptr = (unsigned char *)mmap(0, mfile_size, PROT_READ, MAP_SHARED, file_fd, 0);
                    if (mfile_ptr == (void *)-1)
                    {
                        write_error(resp_fd);
                    }
                    else
                    {
                        write_success(resp_fd);
                    }
                }
                free(filename);
            }
            break;
        case 21:
            if (strcmp("READ_FROM_FILE_OFFSET", field) == 0)
            {
                unsigned int offset, no_bytes;
                read(req_fd, &offset, sizeof(unsigned int));
                read(req_fd, &no_bytes, sizeof(unsigned int));

                write_command(resp_fd, c_length, field);

                if (shm_ptr == NULL || mfile_ptr == NULL || offset < 0 || offset + no_bytes > mfile_size)
                {
                    write_error(resp_fd);
                }
                else
                {
                    memcpy(shm_ptr, mfile_ptr + offset, no_bytes);
                    write_success(resp_fd);
                }
            }
            break;
        case 22:
            if (strcmp("READ_FROM_FILE_SECTION", field) == 0)
            {
                unsigned int section_no, offset, no_bytes;
                read(req_fd, &section_no, sizeof(unsigned int));
                read(req_fd, &offset, sizeof(unsigned int));
                read(req_fd, &no_bytes, sizeof(unsigned int));
                write_command(resp_fd, c_length, field);

                if (mfile_ptr == NULL || shm_ptr == NULL || is_valid_SF(mfile_ptr, mfile_size) != 0)
                {
                    write_error(resp_fd);
                    break;
                }

                short int header_size;
                memcpy(&header_size, mfile_ptr + mfile_size - 4, 2);

                unsigned int header_index = mfile_size - header_size + 3;
                int no_sections = (int)(mfile_ptr[header_index - 1]);
                unsigned int section_offset;
                unsigned int section_size;

                if (section_no > no_sections)
                {
                    write_error(resp_fd);
                    break;
                }

                header_index += 21 * (section_no - 1);

                memcpy(&section_offset, mfile_ptr + header_index + 13, 4);
                memcpy(&section_size, mfile_ptr + header_index + 17, 4);

                if (offset + no_bytes > section_size)
                {
                    write_error(resp_fd);
                    break;
                }
                memcpy(shm_ptr, mfile_ptr + section_offset + offset, no_bytes);
                write_success(resp_fd);
            }
            break;
        case 30:
            if (strcmp("READ_FROM_LOGICAL_SPACE_OFFSET", field) == 0)
            {
                unsigned int logical_offset, no_bytes;
                read(req_fd, &logical_offset, sizeof(unsigned int));
                read(req_fd, &no_bytes, sizeof(unsigned int));

                write_command(resp_fd, c_length, field);

                if (mfile_ptr == NULL || shm_ptr == NULL || is_valid_SF(mfile_ptr, mfile_size) != 0)
                {
                    write_error(resp_fd);
                    break;
                }

                short int header_size;
                memcpy(&header_size, mfile_ptr + mfile_size - 4, 2);

                unsigned int header_index = mfile_size - header_size + 3;
                int no_sections = (int)(mfile_ptr[header_index - 1]);
                int found_sections = 1;
                unsigned int section_offset;
                unsigned int section_size;
                unsigned int section_logical_offset;
                unsigned int current_logical_offset = 0;
                unsigned int next_logical_offset = 0;

                while (next_logical_offset < logical_offset && found_sections <= no_sections)
                {
                    current_logical_offset = next_logical_offset;
                    memcpy(&section_size, mfile_ptr + header_index + 17, 4);

                    next_logical_offset = current_logical_offset + (section_size / LOGICAL_ALIGNMENT + 1) * LOGICAL_ALIGNMENT;
                    header_index += 21;
                    found_sections++;
                }

                if (found_sections - 1 > no_sections)
                {
                    write_error(resp_fd);
                    break;
                }

                header_index -= 21;
                memcpy(&section_offset, mfile_ptr + header_index + 13, 4);
                memcpy(&section_size, mfile_ptr + header_index + 17, 4);
                section_logical_offset = logical_offset - current_logical_offset;

                if (section_logical_offset + no_bytes > section_size)
                {
                    write_error(resp_fd);
                    break;
                }

                memcpy(shm_ptr, mfile_ptr + section_offset + section_logical_offset, no_bytes);
                write_success(resp_fd);
            }
            break;
        default:
            free(field);
            close(file_fd);
            close(shm_fd);
            unlink(filename);
            munmap(shm_ptr, shm_size);
            munmap(mfile_ptr, mfile_size);
            shm_unlink(SHM_NAME);
            unlink(RESP_FIFO_NAME);
        }
    }
    return 0;
}

void write_error(int fd)
{
    char resp_len = 5;
    write(fd, &resp_len, sizeof(char));
    write(fd, "ERROR", resp_len);
}

void write_success(int fd)
{
    char resp_len = 7;
    write(fd, &resp_len, sizeof(char));
    write(fd, "SUCCESS", (int)resp_len);
}

void write_command(int fd, char length, char *command)
{
    write(fd, &length, 1);
    write(fd, command, (int)length);
}

int is_valid_SF(unsigned char *mapped_sf, unsigned int file_size)
{
    short int header_size;
    char magic[3], sect_type;
    short int version;
    magic[2] = 0;
    memcpy(&header_size, mapped_sf + file_size - 4, 2);
    memcpy(magic, mapped_sf + file_size - 2, 2);
    if (strcmp(magic, "oV") != 0)
    {
        printf("Bad magic: %s\n", magic);
        return -1;
    }

    unsigned int header_index = file_size - header_size + 3;
    int no_sections = (int)(mapped_sf[header_index - 1]);
    memcpy(&version, mapped_sf + header_index - 3, 2);

    if (no_sections < 7 || no_sections > 21 || version < 74 || version > 126)
    {
        printf("Bad no_sections or version. both were %d, %d", no_sections, version);
        return -1;
    }

    for (int i = 0; i < no_sections; i++)
    {
        sect_type = (int)(mapped_sf[header_index + 12]);
        if (sect_type != 23 && sect_type != 80 && sect_type != 66 && sect_type != 93)
        {
            printf("Bad sect_type. it was %d on sect %d\n", (int)sect_type, i);
            return -1;
        }
        header_index += 21;
    }
    return 0;
}