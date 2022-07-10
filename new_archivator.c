#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#define UINT64 unsigned long long int

/* [Archive]:
 *      ./new_archivator -a dir dir.arch
 * [Unarchive]:
 *      ./new_archivator -d dir.arch dir
 */

static int get_count(char *dir, DIR *dp);
static void get_length(UINT64 array[], int count, char *dir, DIR *dp);
static void input_length(int arch, UINT64 array[], int count, char *dir, DIR *dp);
static void input_files(int arch, int count, char *dir, DIR *dp);

static void archive(char *dir, char *archName);
static void unarchive(char *dir, char *archName);

int main(const int argc, char* argv[]) {
    char *mode = "-a";
    char *dir;
    char *arch;
    if (argc >= 4) {
        if (strcmp(argv[1], "-d") == 0) {
            mode = "-d";
            dir = argv[3];
            arch = argv[2];
        }
        else {
            dir = argv[2];
            arch = argv[3];
        }
    }
    else
        if (argc == 3) {
            dir = argv[1];
            arch = argv[2];
        }
        else {
            printf("Введите исходный и выходной файлы\n");
            exit(1);
        }

    printf("Папка %s\n", dir);
    printf("Архив %s\n", arch);
    printf("Операция: ");
    if (mode == "-d") printf("разархивирование\n");
    else printf("архивирование\n");

    if (mode == "-d") {
        unarchive(dir, arch);
    }
    else {
        archive(dir, arch);
    }

    printf("done.\n");
    exit(0);
}


static void unarchive(char *dir, char *archName) {
    auto UINT64 now_position = 0;
    auto UINT64 start_position = 0;
    auto int c;
    FILE *arch;
    DIR* dp;//дескриптор папки


    arch = fopen(archName, "rb");//открываем файл-архив на чтение
    if (arch == NULL) return;
    while ((c = getc(arch)) != EOF) {
        start_position++;
        if (c == '\n') break;
    }

    if ((dp = opendir(dir)) == NULL) {
        if (mkdir(dir, S_IRWXU) == -1) {
            fprintf(stderr, "Ошибка создания папки: %s\n", dir);
            return;
        }
        else {
            if ((dp = opendir(dir)) == NULL) {
                fprintf(stderr, "Ошибка открытия папки: %s\n", dir);
                return;
            }
        }
    }
    chdir(dir);

    fseek(arch, 0, SEEK_SET);

    auto char filename[128];
    auto UINT64 filesize;

    auto FILE* file;
    while (fscanf(arch, "| %llu = %s |", &filesize, filename) != 0) {
        file = fopen(filename, "wb");
        if (file == NULL) break;

        printf("|File: %s = Bytes: %llu|\n", filename, filesize);

        now_position = ftell(arch);
        fseek(arch, start_position, SEEK_SET);

        start_position += filesize;
        while (filesize-- > 0)
            putc((c = getc(arch)), file);

        fseek(arch, now_position, SEEK_SET);

        fclose(file);
    }
    closedir(dp);
}

static void archive(char* dir, char* archName) {
    DIR* dp;//дескриптор папки
    int arch;

    arch = open(archName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (arch == -1) {
        fprintf(stderr, "Ошибка открытия файла: %s\n", archName);
        return;
    }

    if ((dp = opendir(dir)) == NULL) {
        fprintf(stderr, "Ошибка открытия папки: %s\n", dir);
        return;
    }
    chdir(dir);

    int count = get_count(dir, dp);//кол-во файлов в папке
    auto UINT64 save_length[count];//кол-во байт в каждом файле
    get_length(save_length, count, dir, dp);//проходимся по всем файлам и записываем длину каждого файла в массив
    input_length(arch, save_length, count, dir, dp);//записываем в файл-архив название каждого файла и его длину
    input_files(arch, count, dir, dp);//записываем в файл-архив данные файлов
    closedir(dp);
}

static int get_count(char* dir, DIR* dp) {
    struct dirent* entry;
    struct stat statbuf;
    int count = 0;

    chdir(dir);

    long int before = telldir(dp);
    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISREG(statbuf.st_mode)) {
            count++;
        }
    }
    seekdir(dp, before);

    return count;
}


static void get_length(UINT64 array[], int count, char* dir, DIR* dp) {
    struct dirent* entry;
    struct stat statbuf;
    chdir(dir);
    int file;
    auto unsigned char index = 0;
    long int before = telldir(dp);

    while ((index < count) && ((entry = readdir(dp)) != NULL)) {
        lstat(entry->d_name, &statbuf);
        if (S_ISREG(statbuf.st_mode)) {
            file = open(entry->d_name, O_RDONLY);
            if (file == -1) continue;

            array[index] = lseek(file, 0, SEEK_END);
            lseek(file, 0, SEEK_SET);

            close(file);

            index++;
        }
    }
    seekdir(dp, before);
}

static void input_length(int arch, UINT64 array[], int count, char* dir, DIR* dp) {
    struct dirent* entry;
    struct stat statbuf;
    char* buffer;
    int len;
    chdir(dir);
    auto unsigned char index = 0;
    long int before = telldir(dp);

    while ((index < count) && ((entry = readdir(dp)) != NULL)) {
        lstat(entry->d_name, &statbuf);
        if (S_ISREG(statbuf.st_mode)) {
            len = sprintf(buffer, "| %llu = %s |", array[index], entry->d_name);
            write(arch, buffer, len*sizeof(char));
            index++;
        }
    }
    len = sprintf(buffer, "\n");
    write(arch, "\n", len * sizeof(char));
    seekdir(dp, before);
}

static void input_files(int arch, int count, char *dir, DIR *dp) {
    struct dirent *entry;
    struct stat statbuf;
    char c;
    chdir(dir);
    int file;
    auto unsigned char index = 0;
    long int before = telldir(dp);

    while ((index < count) && ((entry = readdir(dp)) != NULL)) {
        lstat(entry->d_name, &statbuf);
        if (S_ISREG(statbuf.st_mode)) {
            file = open(entry->d_name, O_RDONLY);
            if (file == -1) continue;

            while (read(file, &c, 1) == 1) write(arch, &c, 1);
            
            close(file);
            index++;
        }
    }
    seekdir(dp, before);
}