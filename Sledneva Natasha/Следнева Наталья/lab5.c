

#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct fs_node_s;

typedef enum node_type {
    FS_NODE_FILE,
    FS_NODE_DIRECTORY
} node_type;


typedef struct file_node {
    int size;
    char *data_ptr;
} fs_file_node_t; /* структура файла */


typedef struct fs_dir_node_s {
    struct fs_node_s *child;
} fs_dir_node_t; /* структура директории */

/* структура для узла в файловом дереве */
typedef struct fs_node_s {
    char *name;
    struct fs_node_s *next_sibling;
    node_type type;/* файл или директория*/
    mode_t mode;/* права доступа */
    uid_t uid;/* id пользователя*/
    gid_t gid;/* id группы пользователя */

    union {
        fs_file_node_t file;/*инфо о файле*/
        fs_dir_node_t dir;/* инфо о директории */
    } info;

} fs_node_t;

static fs_node_t *create_file_with_perm(char *, mode_t);

static fs_node_t *create_directory_with_perm(char *, mode_t);

static fs_node_t *add_file_with_perm(fs_node_t *, char *, mode_t mode);

static fs_node_t *add_directory_with_perm(fs_node_t *, char *, mode_t);

static void add_child_node(fs_node_t *, fs_node_t *);

static fs_node_t *find_node(const char *, fs_node_t *); /* сигнатуры методов */

fs_node_t *root; /* корень дерева */
char testText[77];/* test.txt */
char *pasteBinary; /* paste */

/* Найти узел в файловом дереве по пути к этому файлу или директории */
static fs_node_t *find_node(const char *path, fs_node_t *parent) {
    int entry_len = 0;
    fs_node_t *current, *result = NULL;

    printf("find %s %p\n", path, parent);

    /* пропустить '/' символ */
    while (*path != '\0' && *path == '/') {
        path++;
    }

    /* Посчитать длину текущего пути */
    while (path[entry_len] != '\0' &&
           path[entry_len] != '/') {
        entry_len++;
    }

    if (parent == NULL || parent->type != FS_NODE_DIRECTORY) {
        /* 'parent' должен быть директорией */
        result = parent;
    } else if (entry_len == 0) {
        /* если путь пустой (т.е. "/" или ""), вернуть родителя */
        result = parent;
    } else {
        current = parent->info.dir.child;
        while (current != NULL && strncmp(current->name, path, entry_len)) {
            current = current->next_sibling;
        }

        if (current != NULL) {
            result = find_node(path + entry_len, current);
        }
    }

    return result;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    fs_node_t *parent_node, *current_node;

    printf("do_readdir: %s\n", path);

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    parent_node = find_node(path, root);

    if (parent_node != NULL) {

        current_node = parent_node->info.dir.child;
        while (current_node != NULL) {
            filler(buffer, current_node->name, NULL, 0);
            current_node = current_node->next_sibling;
        }
    } else {
        printf("node not found!\n");
    }

    return 0;
} /* читает директорию */

static int do_getattr(const char *path, struct stat *st) {
    fs_node_t *node;
    int ret = 0;

    printf("do_getattr: %s\n", path);

    node = find_node(path, root);
    if (node != NULL) {
        st->st_mode = node->mode;
        if (node->type == FS_NODE_DIRECTORY) {
            st->st_mode |= S_IFDIR;
        } else if (node->type == FS_NODE_FILE) {
            st->st_mode |= S_IFREG;
            st->st_size = node->info.file.size;
        }

        st->st_uid = node->uid;
        st->st_gid = node->gid;
    } else {
        printf("node not found!");
        ret = -ENOENT;
    }

    return ret;
}/* инфо о файле или директории */

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    fs_node_t *node;
    int bytes_read = 0;

    node = find_node(path, root);
    if (node != NULL && offset < node->info.file.size) {
        printf("read %s\n", node->name);
        if (!strcmp(node->name, "paste")) {
            FILE *fileptr;

            fileptr = fopen("/bin/paste", "rb");  // Open the file in binary mode
            fseek(fileptr, 0, SEEK_END);
            long fsize = ftell(fileptr);
            fseek(fileptr, 0, SEEK_SET);

            pasteBinary = (char *) malloc(fsize);

            fread(pasteBinary, fsize, 1, fileptr);
            fclose(fileptr);

            if (fsize - offset > size) {
                bytes_read = size;
                memcpy(buffer, pasteBinary + offset, size);
            } else {
                bytes_read = fsize - offset;
                memcpy(buffer, pasteBinary + offset, fsize - offset);
            }

        } else {
            int bytes_available = node->info.file.size - offset;
            if (bytes_available >= size) {
                bytes_read = size;
            } else {
                bytes_read = bytes_available;
            }

            if (bytes_read > 0) {
                memcpy(buffer, node->info.file.data_ptr, bytes_read);
            }
        }
    }

    return bytes_read;
} /* читает файл */

static int do_chmod(const char *path, mode_t mode) {
    fs_node_t *node = find_node(path, root);
    if (node != NULL) {
        node->mode = mode;/* обращение к полю mode структуры node и изменение его на новое значение */
        return 0;
    }
    return -1;
}/* меняет права доступа */

static struct fuse_operations operations = {
        .getattr    = do_getattr,
        .readdir    = do_readdir,
        .read         = do_read,
        .chmod = do_chmod
};

static fs_node_t *create_directory_with_perm(char *name, mode_t mode) {
    fs_node_t *node;

    node = (fs_node_t *) malloc(sizeof(fs_node_t));

    /* установить значения по умолчанию */
    node->name = name;
    node->mode = mode;
    node->type = FS_NODE_DIRECTORY;
    node->next_sibling = NULL;
    node->info.dir.child = NULL;
    node->gid = getgid();
    node->uid = getuid();

    return node;
}

static fs_node_t *add_directory_with_perm(fs_node_t *root, char *name, mode_t mode) {
    fs_node_t *dir = create_directory_with_perm(name, mode);
    add_child_node(root, dir);
    return dir;
}

static fs_node_t *create_file_with_perm(char *name, mode_t mode) {
    fs_node_t *node;

    node = (fs_node_t *) malloc(sizeof(fs_node_t));

    /* установить значения по умолчанию */
    node->name = name;
    node->mode = mode;
    node->type = FS_NODE_FILE;
    node->next_sibling = NULL;
    node->info.file.size = 0;
    node->info.file.data_ptr = NULL;
    node->gid = getgid();
    node->uid = getuid();

    return node;
}

static fs_node_t *add_file_with_perm(fs_node_t *root, char *name, mode_t mode) {
    fs_node_t *file = create_file_with_perm(name, mode);
    add_child_node(root, file);
    return file;
}

static void add_child_node(fs_node_t *parent, fs_node_t *child) {

    child->next_sibling = parent->info.dir.child;
    parent->info.dir.child = child;
}

static void generate_tree() {
    fs_node_t *bar, *bin, *paste, *readme, *foo, *test, *baz, *example;

    //---------------директории
    foo = add_directory_with_perm(root, "foo", 0333);
    baz = add_directory_with_perm(foo, "baz", 0644);
    bin = add_directory_with_perm(bar, "bin", 0700);
    bar = add_directory_with_perm(root, "bar", 0755);

    paste = add_file_with_perm(bin, "paste", 0555);
    readme = add_file_with_perm(bin, "readme.txt", 0400);
    test = add_file_with_perm(foo, "test.txt", 0707);
    example = add_file_with_perm(baz, "example", 0211);

    //-------------------файлы
    readme->info.file.data_ptr = "Student Наташа Следнева, 16150038\n\0";
    readme->info.file.size = strlen(readme->info.file.data_ptr);

    //------------------------------
    for (int i = 0; i < 38; i++) {
        testText[i * 2] = 'v';
        testText[i * 2 + 1] = '\n';
    }
    testText[76] = '\0';
    test->info.file.data_ptr = testText;
    test->info.file.size = strlen(testText);

    //--------------------------
    example->info.file.data_ptr = "Hello world\n\0";
    example->info.file.size = strlen(example->info.file.data_ptr);

    //-------------------
    FILE *fileptr;

    fileptr = fopen("/bin/paste", "rb");  // открыть файл в бинарном режиме
    fseek(fileptr, 0, SEEK_END);
    long fsize = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);

    paste->info.file.size = fsize;
}

int main(int argc, char *argv[]) {
    root = create_directory_with_perm("", 0666);
    generate_tree();

    return fuse_main(argc, argv, &operations, NULL);
}
