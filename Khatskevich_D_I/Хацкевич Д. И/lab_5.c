

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
} fs_file_node_t; /* file structure */


typedef struct fs_dir_node_s {
    struct fs_node_s *child;
} fs_dir_node_t; /*directory structure*/

/* structure representing a node in our FS tree */
typedef struct fs_node_s {
    char *name;
    struct fs_node_s *next;
    node_type type;/*file or directory*/
    mode_t mode;/*permissions*/
    uid_t uid;/* user id*/
    gid_t gid;/* group id*/

    union {
        fs_file_node_t file;/*info about file*/
        fs_dir_node_t dir;/*info about dir*/
    } info;

} fs_node_t;

static fs_node_t *find_node(const char *, fs_node_t *);

static fs_node_t *create_directory_with_permission(char *, mode_t);

static fs_node_t *create_file_with_permission(char *, mode_t);

static fs_node_t *add_directory_with_permission(fs_node_t *, char *, mode_t);

static fs_node_t *add_file_with_permission(fs_node_t *, char *, mode_t mode);

static void add_child_node(fs_node_t *, fs_node_t *);


fs_node_t *root;
char testText[89];/*test.txt*/
char *cutBinary; /*cut*/

/* Find a node in our virtual FS tree corresponding to the specified path. */
static fs_node_t *find_node(const char *path, fs_node_t *parent) {
    int entry_len = 0;
    fs_node_t *current, *result = NULL;

    printf("find %s %p\n", path, parent);

    /* skip leading '/' symbols */
    while (*path != '\0' && *path == '/') {
        path++;
    }

    /* calculate the length of the current path entry */
    while (path[entry_len] != '\0' &&
           path[entry_len] != '/') {
        entry_len++;
    }

    if (parent == NULL || parent->type != FS_NODE_DIRECTORY) {
        /* 'parent' must represent a directory */
        result = parent;
    } else if (entry_len == 0) {
        /* if the path is empty (e.g. "/" or ""), return parent */
        result = parent;
    } else {
        /* traverse children in search for the next entry */
        current = parent->info.dir.child;
        while (current != NULL && strncmp(current->name, path, entry_len)) {
            current = current->next;
        }

        if (current != NULL) {
            result = find_node(path + entry_len, current);
        }
    }

    return result;
}

static int _read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    fs_node_t *node;
    int bytes_read = 0;
    
    node = find_node(path, root);
    if (node != NULL && offset < node->info.file.size) {
        printf("read %s\n", node->name);
        if (!strcmp(node->name, "cut")) {
            FILE *fileptr;
            
            fileptr = fopen("/bin/cut", "rb");  // Open the file in binary mode
            fseek(fileptr, 0, SEEK_END);
            long fsize = ftell(fileptr);
            fseek(fileptr, 0, SEEK_SET);
            
            cutBinary = (char *) malloc(fsize);
            
            fread(cutBinary, fsize, 1, fileptr);
            fclose(fileptr);
            
            if (fsize - offset > size) {
                bytes_read = size;
                memcpy(buffer, cutBinary + offset, size);
            } else {
                bytes_read = fsize - offset;
                memcpy(buffer, cutBinary + offset, fsize - offset);
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
}


static int _readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    fs_node_t *parent_node, *current_node;

    printf("_readdir: %s\n", path);

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    parent_node = find_node(path, root);

    if (parent_node != NULL) {

        current_node = parent_node->info.dir.child;
        while (current_node != NULL) {
            filler(buffer, current_node->name, NULL, 0);
            current_node = current_node->next;
        }
    } else {
        printf("node not found!\n");
    }

    return 0;
}

static int _chown(const char *path, uid_t uid, gid_t gid) {
    fs_node_t *node = find_node(path, root);
    if (node != NULL) {
        node->uid = uid;
        node->gid = gid;
        return 0;
    }
    return -1;
}

static int _getattr(const char *path, struct stat *st) {
    fs_node_t *node;
    int ret = 0;

    printf("_getattr: %s\n", path);

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
}/*info about file or dir*/


static struct fuse_operations operations = {
        .read         = _read,
        .readdir    = _readdir,
        .chown = _chown,
        .getattr    = _getattr
};

static fs_node_t *create_directory_with_permission(char *name, mode_t mode) {
    fs_node_t *node;

    node = (fs_node_t *) malloc(sizeof(fs_node_t));

    /* set default values */
    node->name = name;
    node->mode = mode;
    node->type = FS_NODE_DIRECTORY;
    node->next = NULL;
    node->info.dir.child = NULL;
    node->gid = getgid();
    node->uid = getuid();

    return node;
}

static fs_node_t *create_file_with_permission(char *name, mode_t mode) {
    fs_node_t *node;
    
    node = (fs_node_t *) malloc(sizeof(fs_node_t));
    
    /* set default values */
    node->name = name;
    node->mode = mode;
    node->type = FS_NODE_FILE;
    node->next = NULL;
    node->info.file.size = 0;
    node->info.file.data_ptr = NULL;
    node->gid = getgid();
    node->uid = getuid();
    
    return node;
}

static fs_node_t *add_directory_with_permission(fs_node_t *root, char *name, mode_t mode) {
    fs_node_t *dir = create_directory_with_permission(name, mode);
    add_child_node(root, dir);
    return dir;
}


static fs_node_t *add_file_with_permission(fs_node_t *root, char *name, mode_t mode) {
    fs_node_t *file = create_file_with_permission(name, mode);
    add_child_node(root, file);
    return file;
}

static void add_child_node(fs_node_t *parent, fs_node_t *child) {
    /* assuming parent is always a directory */

    child->next = parent->info.dir.child;
    parent->info.dir.child = child;
}

static void generate_fs_tree() {
    fs_node_t *foo, *bar, *bin, *baz, *cut, *readme, *test, *example;

    //---------------directories
    foo = add_directory_with_permission(root, "foo", 0441);
    bar = add_directory_with_permission(foo, "bar", 0664);
    bin = add_directory_with_permission(root, "bin", 0700);
    baz = add_directory_with_permission(foo, "baz", 0244);
    
    //-------------------files
    cut = add_file_with_permission(bin, "cut", 0700);
    readme = add_file_with_permission(foo, "readme.txt", 0411);
    test = add_file_with_permission(foo, "test.txt", 0000);
    example = add_file_with_permission(foo, "example", 0200);


    readme->info.file.data_ptr = "Student Даниил Хацкевич, 16150044\n\0";
    readme->info.file.size = strlen(readme->info.file.data_ptr);


    for (int i = 0; i < 44; i++) {
        testText[i * 2] = 'k';
        testText[i * 2 + 1] = '\n';
    }
    testText[88] = '\0';
    test->info.file.data_ptr = testText;
    test->info.file.size = strlen(testText);

   
    example->info.file.data_ptr = "Hello world\n\0";
    example->info.file.size = strlen(example->info.file.data_ptr);


    FILE *fileptr;

    fileptr = fopen("/bin/cut", "rb");  // Open the file in binary mode
    fseek(fileptr, 0, SEEK_END);
    long fsize = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);

    cut->info.file.size = fsize;
}
    //-----------------------------------

int main(int argc, char *argv[]) {
    root = create_directory_with_permission("", 0777);
    generate_fs_tree();

    return fuse_main(argc, argv, &operations, NULL);
}
