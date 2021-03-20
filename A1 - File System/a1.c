#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#pragma pack(1)
typedef struct header
{
    char name[20];
    char type;
    int offset;
    int size;
} header;
#pragma pack(4)

int parseSF(char *filepath, int silent); //silent option disables prints
void findAll(char *path); //list recursive where we only list SFs with >= 14 lines for at least one section
void findAll_utility(DIR *dir, char *path);
int isGoodSF(char *path); //checks for SFs with >=14 lines
int list(char *path, int recursive, char *string, char checkPerm); //if string is passed as null, it will list with option has_perm_execute
int list_utility(DIR *dir, char *path, int recursive, char *string, char checkPerm);
void extract(char *path, int section, int line);

void parseListCommand(char **argv, int argc);
void parseParseCommand(char **argv, int argc);
void parseFindAllCommand(char **argv, int argc);
void parseExtractCommand(char **argv, int argc);

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            printf("76954\n");
        }
    }
    else
    {
        if (0 == strcmp(argv[1], "list"))
            parseListCommand(argv, argc);
        if (0 == strcmp(argv[1], "parse"))
            parseParseCommand(argv, argc);
        if (0 == strcmp(argv[1], "extract"))
            parseExtractCommand(argv, argc);
        if (0 == strcmp(argv[1], "findall"))
            parseFindAllCommand(argv, argc);
    }
    return 0;
}

void parseListCommand(char **argv, int argc)
{
    char path[256];
    char *buf = calloc(256, sizeof(char));
    if (argc == 3) // list no filter
    {
        sscanf(argv[2], "path=%s", path);
        list(path, 0, NULL, 'n');
    }
    else if (argc == 4)
    { //list with filter no recursive  | list recursive no filter

        int k = 2; //index of recursive or filter
        if (strstr(argv[2], "path="))
        {
            sscanf(argv[2], "path=%s", path);
            k = 3;
        }
        else
            sscanf(argv[3], "path=%s", path);

        if (strstr(argv[k], "name_ends_with="))
        {
            sscanf(argv[k], "name_ends_with=%s", buf);
            list(path, 0, buf, 'n');
        }
        else if (strcmp(argv[k], "has_perm_execute") == 0)
        {
            list(path, 0, NULL, 'y');
        }
        else if (strcmp(argv[k], "recursive") == 0)
        {
            list(path, 1, NULL, 'n');
        }
        else
        {
            printf("Bad list command.\n");
        }
    }
    else if (argc == 5)
    { // list recursive with filter
        int filter_index = 2, recursive = 0;

        if (strstr(argv[2], "path="))
        {
            sscanf(argv[2], "path=%s", path);
            if (0 == strcmp(argv[3], "recursive"))
            {
                filter_index = 4;
                recursive = 1;
            }
            else if (0 == strcmp(argv[4], "recursive"))
            {
                recursive = 1;
                filter_index = 3;
            }
        }
        else
        {
            if (strstr(argv[3], "path="))
            {
                sscanf(argv[3], "path=%s", path);
                if (0 == strcmp(argv[2], "recursive"))
                {
                    filter_index = 4;
                    recursive = 1;
                }
                else if (0 == strcmp(argv[4], "recursive"))
                {
                    filter_index = 2;
                    recursive = 1;
                }
            }
            else
            {
                if (strstr(argv[4], "path="))
                {
                    sscanf(argv[4], "path=%s", path);
                    if (0 == strcmp(argv[2], "recursive"))
                    {
                        filter_index = 3;
                        recursive = 1;
                    }
                    else if (0 == strcmp(argv[3], "recursive"))
                    {
                        filter_index = 2;
                        recursive = 1;
                    }
                }
                else
                    printf("Bad list command.\n");
            }
        }
        if (recursive == 0)
            printf("Bad list command.\n");
        if (strstr(argv[filter_index], "name_ends_with="))
        {
            sscanf(argv[filter_index], "name_ends_with=%s", buf);
            list(path, 1, buf, 'n');
        }
        else if (strcmp(argv[filter_index], "has_perm_execute") == 0)
        {
            list(path, 1, NULL, 'y');
        }
        else
        {
            printf("Bad list command.\n");
        }
    }
    else
        printf("Bad list command.\n");
    free(buf);
    return;
}

void parseParseCommand(char **argv, int argc)
{
    char path[256];
    if (argc == 3 && NULL != strstr(argv[2], "path="))
    {
        sscanf(argv[2], "path=%s", path);
        parseSF(path, 0);
    }
}

void parseFindAllCommand(char **argv, int argc)
{
    char path[256];
    if (argc == 3 && NULL != strstr(argv[2], "path="))
    {
        sscanf(argv[2], "path=%s", path);
        findAll(path);
    }
}

void parseExtractCommand(char **argv, int argc)
{
    int section, line;
    int lineIndex;
    char path[256];
    if (argc != 5)
        printf("Bad extract command.\n");
    if (strstr(argv[2], "path="))
    {
        sscanf(argv[2], "path=%s", path);
        if (strstr(argv[3], "section="))
        {
            sscanf(argv[3], "section=%d", &section);
            lineIndex = 4;
        }
        else if (strstr(argv[4], "section="))
        {
            sscanf(argv[4], "section=%d", &section);
            lineIndex = 3;
        }
    }
    else if (strstr(argv[3], "path="))
    {
        sscanf(argv[3], "path=%s", path);
        if (strstr(argv[2], "section="))
        {
            sscanf(argv[2], "section=%d", &section);
            lineIndex = 4;
        }
        else if (strstr(argv[4], "section="))
        {
            sscanf(argv[4], "section=%d", &section);
            lineIndex = 2;
        }
    }
    else if (strstr(argv[4], "path="))
    {
        sscanf(argv[4], "path=%s", path);
        if (strstr(argv[2], "section="))
        {
            sscanf(argv[2], "section=%d", &section);
            lineIndex = 3;
        }
        else if (strstr(argv[3], "section="))
        {
            sscanf(argv[3], "section=%d", &section);
            lineIndex = 2;
        }
    }
    else
        printf("Bad extract command.\n");
    if (strstr(argv[lineIndex], "line="))
    {
        sscanf(argv[lineIndex], "line=%d", &line);
    }
    else
        printf("Bad extract command.\n");

    if (parseSF(path, 1) != 0)
        printf("ERROR\ninvalid file");
    else
        extract(path, section, line);
}

int parseSF(char *filepath, int silent)
{ 
    int fd = open(filepath, O_RDONLY);
    if (fd == -1)
    {
        if (!silent)
            printf("ERROR\nCould not open file.\n");
        return -1;
    }
    char version, no_sections;
    short header_size;
    char *magic = "yFUj";
    char *buf = calloc(256, sizeof(char));
    lseek(fd, -6, SEEK_END);
    if (read(fd, buf, 2) == -1)
    {
        if (!silent)
            printf("ERROR\nCouldn't read header size\n");
        free(buf);
        close(fd);
        return -1;
    }

    memcpy(&header_size, buf, 2);

    if (read(fd, buf, 4) == -1 || strcmp(buf, magic))
    {
        if (!silent)
            printf("ERROR\nwrong magic\n");
        free(buf);
        close(fd);
        return -1;
    }
    lseek(fd, -header_size, SEEK_END);
    if (read(fd, buf, 2) == -1)
    {
        if (!silent)
            printf("ERROR\nCouldn't read version and no_of_sections");
        free(buf);
        close(fd);
        return -1;
    }
    sscanf(buf, "%c%c", &version, &no_sections);
    if (version < 26 || version > 120)
    {
        if (!silent)
            printf("ERROR\nwrong version");
        free(buf);
        close(fd);
        return -1;
    }
    if (no_sections < 2 || no_sections > 10)
    {
        if (!silent)
            printf("ERROR\nwrong sect_nr");
        free(buf);
        close(fd);
        return -1;
    }

    lseek(fd, 20, SEEK_CUR);

    char sect_type;

    for (int i = 0; i < no_sections; i++)
    {
        if (read(fd, &sect_type, 1) == -1 || !(sect_type == 10 || sect_type == 16 || sect_type == 35 || sect_type == 48 || sect_type == 56 || sect_type == 72))
        {
            if (!silent)
                printf("ERROR\nwrong sect_types");
            free(buf);
            close(fd);
            return -1;
        }
        lseek(fd, 28, SEEK_CUR);
    }
    int iversion, isections;
    iversion = version;
    isections = no_sections;
    if (!silent)
        printf("SUCCESS\nversion=%d\nnr_sections=%d\n", iversion, isections);
    else
    {
        free(buf);
        close(fd);
        return 0;
    }

    header h;
    int type;

    lseek(fd, -header_size + 2, SEEK_END);
    for (int i = 1; i <= no_sections; i++)
    {
        read(fd, &h, 29);
        memcpy(buf, &h.name, 20); //buf assures null-terminated string
        type = h.type;
        printf("section%d: %s %d %d\n", i, buf, type, h.size);
    }
    free(buf);
    close(fd);
    return 0;
}

int list(char *path, int recursive, char *string, char checkPerm)
{
    DIR *dir = NULL;
    dir = opendir(path);
    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path");
        return -1;
    }
    printf("SUCCESS\n");
    return list_utility(dir, path, recursive, string, checkPerm);
}

int list_utility(DIR *dir, char *path, int recursive, char *string, char checkPerm)
{
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);

            if (lstat(fullPath, &statbuf) == 0)
            {
                if (string)
                { //option was name_ends_with
                    int slen = strlen(string);
                    int nlen = strlen(entry->d_name);
                    if (0 == strcmp(entry->d_name + (nlen - slen), string))
                    {
                        printf("%s", fullPath);
                        printf("\n");
                    }
                }
                else
                { //option has_perm_execute or no filter
                    if (checkPerm == 'n' || S_IXUSR & statbuf.st_mode)
                    {
                        printf("%s", fullPath);
                        printf("\n");
                    }
                }
                if (S_ISDIR(statbuf.st_mode) && recursive)
                {
                    DIR *dir2 = NULL;
                    dir2 = opendir(fullPath);
                    if (dir2 == NULL)
                    {
                        perror("ERROR\ninvalid directory path");
                        return -1;
                    }
                    list_utility(dir2, fullPath, recursive, string, checkPerm);
                }
            }
        }
    }
    closedir(dir);
    return 0;
}

void extract(char *path, int section, int line)
{

    int fd = open(path, O_RDONLY);
    char *buf = calloc(256, sizeof(char));

    short header_size;
    char no_sections;

    lseek(fd, -6, SEEK_END);
    read(fd, buf, 2);

    memcpy(&header_size, buf, 2);

    lseek(fd, -header_size + 1, SEEK_END);
    read(fd, buf, 1);
    sscanf(buf, "%c", &no_sections);

    if (section > no_sections || section < 1)
    {
        printf("ERROR\ninvalid section");
        free(buf);
        close(fd);
        return;
    }

    header h;
    for (int i = 1; i <= section; i++)
    {
        if (read(fd, &h, sizeof(header)) == -1)
        {
            printf("ERROR while reading file header.\n");
            free(buf);
            close(fd);
        }
    }

    lseek(fd, h.offset + h.size, SEEK_SET); //seek to the end of the section

    int linesRead = 0;
    int lineLength = 0;
    int unreadSize = h.size;
    int endline = 0;
    int found = 0;
    int lastIndex = 255;

    while (unreadSize > 255 && !found)
    {
        lseek(fd, -255, SEEK_CUR);
        read(fd, buf, 255);
        unreadSize -= 255;
    
        char *p = strrchr(buf, 0x0A);
        if (!p)
            endline += 255;
        else
        {
            while (p)
            {
                linesRead++;

                if (linesRead == line)
                {
                    lseek(fd, -255 + p - buf + 1, SEEK_CUR); //move pointer to line start position, after 0x0A
                    lineLength = endline + lastIndex - (p - buf);
                    found = 1;
                    break;
                }
                else
                {
                    lastIndex = p - buf;
                    buf[lastIndex] = 0x0;
                    endline = 0;
                }
                p = strrchr(buf, 0x0A);
            }
            endline = lastIndex;
            lastIndex = 255;
        }
        if(!found) lseek(fd, -255, SEEK_CUR);
    }
    if (!found)
    {
        lseek(fd, h.offset, SEEK_SET); 
        read(fd, buf, unreadSize);  
        buf[unreadSize] = 0x0;
        lastIndex = unreadSize;

        char *p = strrchr(buf, 0x0A);
        
        while (p)
        {
            linesRead++;
            if (linesRead == line)
            {
                lseek(fd, -unreadSize + p - buf + 1, SEEK_CUR); //move pointer to current position, after 0x0A
                lineLength = endline + lastIndex - (p - buf);
                found = 1;
                break;
            }
            else
            {
                lastIndex = p - buf;
                buf[lastIndex] = 0x0;
                endline = 0;
            }
            p = strrchr(buf, 0x0A);
        }
        if (!p && linesRead == line - 1) {
            endline += lastIndex;
            lineLength = endline;
            lseek(fd, -unreadSize, SEEK_CUR);
            found = 1;
        }
    }
    if (!found)
    {
        printf("ERROR\ninvalid line");
        free(buf);
        close(fd);
        return;
    }

    printf("SUCCESS\n");
    
    unreadSize = lineLength; //try implementing it without lineLength logic?
    while (unreadSize > 255)
    {
        read(fd, buf, 255);
        unreadSize -= 255;
        printf("%s", buf);
    }
    read(fd, buf, unreadSize);
    buf[unreadSize] = '\0';
    
    printf("%s", buf);
    close(fd);
    free(buf);
}

void findAll(char *path)
{
    DIR *dir = NULL;
    dir = opendir(path);
    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path");
        return;
    }
    printf("SUCCESS\n");
    findAll_utility(dir, path);
}

void findAll_utility(DIR *dir, char *path)
{
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);

            if (lstat(fullPath, &statbuf) == 0)
            {
                if (S_ISREG(statbuf.st_mode) && 0 == isGoodSF(fullPath))
                { //option was name_ends_with
                    printf("%s", fullPath);
                    printf("\n");
                }
                if (S_ISDIR(statbuf.st_mode))
                {
                    DIR *dir2 = NULL;
                    dir2 = opendir(fullPath);
                    if (dir2 == NULL)
                    {
                        perror("ERROR\ninvalid directory path");
                        return;
                    }
                    findAll_utility(dir2, fullPath);
                }
            }
        }
    }
    closedir(dir);
}

int isGoodSF(char *path)
{
    if (0 != parseSF(path, 1))
        return -1;
    int fd = open(path, O_RDONLY);
    char *buf = calloc(256, sizeof(char));

    short header_size;
    char no_sections;

    lseek(fd, -6, SEEK_END);
    read(fd, buf, 2);

    memcpy(&header_size, buf, 2);

    lseek(fd, -header_size + 1, SEEK_END);
    read(fd, buf, 1);
    sscanf(buf, "%c", &no_sections);

    header h;
    int currentPosition, noLines;
    for (int i = 0; i < no_sections; i++)
    {
        if (read(fd, &h, sizeof(header)) == -1)
        {
            printf("ERROR while reading file header.\n");
            free(buf);
            close(fd);
        }
        currentPosition = lseek(fd, 0, SEEK_CUR);
        noLines = 0;
        lseek(fd, h.offset, SEEK_SET);

        int unreadSize = h.size;
        while (unreadSize > 255)
        {
            read(fd, buf, 255);
            unreadSize -= 255;
            char *p = strchr(buf, 0x0A);
            while (p)
            {
                if (p != buf + 255)
                {
                    noLines++;
                    if (noLines >= 14)
                    {
                        free(buf);
                        close(fd);
                        return 0;
                    }
                    p = strchr(p + 1, 0x0A);
                }
                else
                    break;
            }
        }
        read(fd, buf, unreadSize);
        buf[unreadSize] = '\0';
        char *p = strchr(buf, 0x0A);
        while (p)
        {
            if (p != buf + unreadSize)
            {
                noLines++;
                if (noLines >= 14)
                {
                    free(buf);
                    close(fd);
                    return 0;
                }
                p = strchr(p + 1, 0x0A);
            }
            else
                break;
        }
        lseek(fd, currentPosition, SEEK_SET);
    }
    free(buf);
    close(fd);
    return -1;
}