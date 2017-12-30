/*
 /
 |--bin/777
 |  `--ls/777
 |--bar/755
 |   `--baz/744
 |       |--readme.txt/444
 |       `--example/200
 `--foo/111
    `--test.txt/000

 Дополнительно должна быть реализована функция `write`.
*/

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

//Файл
typedef struct file_node {
    int size;
    char *data_ptr;
} fs_file_node_t; 

//Директория
typedef struct fs_dir_node_s {
    struct fs_node_s *child;
} fs_dir_node_t; 


typedef struct fs_node_s {
    char *name;
    struct fs_node_s *next_sibling;
    node_type type;		//file or directory
    mode_t mode;		//permissions
    uid_t uid;			//user id
    gid_t gid;			//group id

    union {
        fs_file_node_t file;	//info about file
        fs_dir_node_t dir;		//info about dir
    } info;

} fs_node_t;

static fs_node_t *create_file_with_perm(char *, mode_t);

static fs_node_t *create_directory_with_perm(char *, mode_t);

static fs_node_t *add_file_with_perm(fs_node_t *, char *, mode_t mode);

static fs_node_t *add_directory_with_perm(fs_node_t *, char *, mode_t);

static void add_child_node(fs_node_t *, fs_node_t *);

static fs_node_t *find_node(const char *, fs_node_t *); /*сигнатуры методов*/

fs_node_t *root;	//корень дерева
char testText[53];	//test.txt
char *lsBinary;		//ls

// поиск узла по пути
static fs_node_t *find_node(const char *path, fs_node_t *parent) {
    int entry_len = 0;
    fs_node_t *current, *result = NULL;

    printf("find %s %p\n", path, parent);

    while (*path != '\0' && *path == '/') {
        path++;
    }

    while (path[entry_len] != '\0' &&
           path[entry_len] != '/') {
        entry_len++;
    }

    if (parent == NULL || parent->type != FS_NODE_DIRECTORY) {
        result = parent;
    } else if (entry_len == 0) {
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

static int do_getattr(const char *path, struct stat *st) {
	fs_node_t *node;
	int ret = 0;

	printf("do_getattr: %s\n", path);

	node = find_node(path, root);
	if (node != NULL) {
		st->st_mode = node->mode;
		if (node->type == FS_NODE_DIRECTORY) {
			st->st_mode |= S_IFDIR;
		}
		else if (node->type == FS_NODE_FILE) {
			st->st_mode |= S_IFREG;
			st->st_size = node->info.file.size;
		}

		st->st_uid = node->uid;
		st->st_gid = node->gid;
	}
	else {
		printf("node not found!");
		ret = -ENOENT;
	}

	return ret;
}//info about file or dir

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
}



static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    fs_node_t *node;
    int bytes_read = 0;

    node = find_node(path, root);
    if (node != NULL && offset < node->info.file.size) {
        printf("read %s\n", node->name);
        if (!strcmp(node->name, "ls")) {
            FILE *fileptr;

            fileptr = fopen("/bin/ls", "rb");  // Open the file in binary mode
            fseek(fileptr, 0, SEEK_END);
            long fsize = ftell(fileptr);
            fseek(fileptr, 0, SEEK_SET);

            lsBinary = (char *) malloc(fsize);

            fread(lsBinary, fsize, 1, fileptr);
            fclose(fileptr);

            if (fsize - offset > size) {
                bytes_read = size;
                memcpy(buffer, lsBinary + offset, size);
            } else {
                bytes_read = fsize - offset;
                memcpy(buffer, lsBinary + offset, fsize - offset);
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


static int do_write(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	fs_node_t *node = find_node(path, root);
	if (node != NULL ) 
	{
		if(offset + size <= node->info.file.size)
		{ 
			for(int i=0;i<size;i++)
			{
				node->info.file.data_ptr[offset+i] = buffer[i];
			}
			return size;
		}else	
		{
			char *temp = (char*)malloc(offset + size);
			for(int i=0;i<node->info.file.size;i++)
			{
				temp[i] = node->info.file.data_ptr[i];
			}

			for(int i=0;i<size;i++)
			{
				temp[offset+i] = buffer[i];
			}
			// free old memory block
			node->info.file.data_ptr = temp;
			node->info.file.size = offset + size;
			return size;
		}
	}
    return -1;
} // write

static int do_open(const char *path, struct fuse_file_info *fi){
	return 0;
}

*/
//заглушки
int my_setxattr (const char *b, const char *c, const char *d, size_t e, int f){
	return 0;
}

int my_chown (const char *a, uid_t b, gid_t c){
	return 0;
}
int my_chmod (const char *a, mode_t b){
	return 0;
}
int my_truncate (const char *a, off_t b){
	return 0;
}
int my_utime (const char *a, struct utimbuf *b){
	return 0;
}


static struct fuse_operations operations = {
        .getattr    = do_getattr,
		.setxattr   = my_setxattr,
		.chmod		= my_chown,
		.chown		= my_chmod,
		.truncate	= my_truncate,
		.utime		= my_utime,
		.open       = do_open,
        .readdir    = do_readdir,
        .read       = do_read,
        .write      = do_write
};

static fs_node_t *create_directory_with_perm(char *name, mode_t mode) {
    fs_node_t *node;

    node = (fs_node_t *) malloc(sizeof(fs_node_t));

    /* set default values */
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
    fs_node_t *bar, *bin, *ls, *readme, *foo, *test, *baz, *example;
   
	bin = add_directory_with_perm(root, "bin", 0777);				// dir
		ls = add_file_with_perm(bin, "ls", 0777);					//file
	bar = add_directory_with_perm(root, "bar", 0755);				// dir
		baz = add_directory_with_perm(bar, "baz", 0744);			// dir
			readme = add_file_with_perm(baz, "readme.txt", 0777);	//file (поменяли, чтобы можно было переписать файл)
			example = add_file_with_perm(baz, "example", 0200);		//file
	foo = add_directory_with_perm(root, "foo", 0111);				// dir
		test = add_file_with_perm(foo, "test.txt", 0000);			//file		

    //---Student <имя и фамилия>, <номер зачетки>
    //malloc, strcpy
    readme->info.file.data_ptr = (char*)malloc(100);
    strcpy(readme->info.file.data_ptr, "Student Модестов Максим, 16150026\n\0");
    readme->info.file.size = strlen(readme->info.file.data_ptr);

    //---<Любой текст на ваш выбор с количеством строк равным последним двум цифрам номера зачетки>
    for (int i = 0; i < 26; i++) {
        testText[i * 2] = 'q';
        testText[i * 2 + 1] = '\n';
    }
    testText[52] = '\0';
    test->info.file.data_ptr = testText;
    test->info.file.size = strlen(testText);

    //---Hello world
    example->info.file.data_ptr = "Hello world\n\0";
    example->info.file.size = strlen(example->info.file.data_ptr);

    //---содержимое бинарного файла должно быть взято из соответствующей стандартной системной утилиты
    FILE *fileptr;

    fileptr = fopen("/bin/ls", "rb");  // Open the file in binary mode
    fseek(fileptr, 0, SEEK_END);
    long fsize = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);

    ls->info.file.size = fsize;
}

int main(int argc, char *argv[]) {
    root = create_directory_with_perm("", 0666);
    generate_tree();

    return fuse_main(argc, argv, &operations, NULL);
}
