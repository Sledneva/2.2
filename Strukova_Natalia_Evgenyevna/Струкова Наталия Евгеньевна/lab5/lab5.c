/*
	/
	|--bin/755
	|   `--bar/777
	`--foo/555
	   `--baz/444
		  |--env/767
		  |--example/277
		  |--readme.txt/426
		  `--test.txt/665

Дополнительно должна быть реализована функция `mkdir`.*/

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
} node_type;//2 типа в ФС - файлы и директории


typedef struct file_node {
	int size;
	char *data_ptr;
} fs_file_node_t; //у файла есть размер и данные


typedef struct fs_dir_node_s {
	struct fs_node_s *child;
} fs_dir_node_t; //у директории есть указатель, на первый эллемент из списка потомков

typedef struct fs_node_s {
	char *name;
	struct fs_node_s *next_sibling;
	node_type type;//тип узла
	mode_t mode;//права
	uid_t uid;//id владельца
	gid_t gid;//id группы

	union {
		fs_file_node_t file;//инфа о файле
		fs_dir_node_t dir;//о директории
	} info;
} fs_node_t;//структура узла

static fs_node_t *create_file(char *, mode_t);//синтаксис методов
static fs_node_t *create_directory(char *, mode_t);
static fs_node_t *add_file(fs_node_t *, char *, mode_t mode);
static fs_node_t *add_directory(fs_node_t *, char *, mode_t);
static fs_node_t *find_node(const char *, fs_node_t *);
static void add_child_node(fs_node_t *, fs_node_t *);

static fs_node_t *create_file(char *name, mode_t mode) {
	fs_node_t *node;
	node = (fs_node_t *)malloc(sizeof(fs_node_t));
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

static fs_node_t *create_directory(char *name, mode_t mode) {
	fs_node_t *node;
	node = (fs_node_t *)malloc(sizeof(fs_node_t));
	node->name = name;
	node->mode = mode;
	node->type = FS_NODE_DIRECTORY;
	node->next_sibling = NULL;
	node->info.dir.child = NULL;
	node->gid = getgid();
	node->uid = getuid();
	return node;
}

static fs_node_t *add_directory(fs_node_t *root, char *name, mode_t mode) {
	fs_node_t *dir = create_directory(name, mode);
	add_child_node(root, dir);
	return dir;
}

static fs_node_t *add_file(fs_node_t *root, char *name, mode_t mode) {
	fs_node_t *file = create_file(name, mode);
	add_child_node(root, file);
	return file;
}

static void add_child_node(fs_node_t *parent, fs_node_t *child) {
	child->next_sibling = parent->info.dir.child;
	parent->info.dir.child = child;
}

fs_node_t *root; //корень
char Text[256];//файл, который мы сформируем
char *env; //бинарный файл


static fs_node_t *find_node(const char *path, fs_node_t *parent) { //поиск вершины по пути
	int len = 0;//длинна записи
	fs_node_t *current, *result = NULL;//указатели на текущий узел и на результат
	printf("find %s %p\n", path, parent);
	while (*path != '\0' && *path == '/') {
		path++;
	}
	while (path[len] != '\0' && path[len] != '/') {
		len++;
	}
	if (parent == NULL || parent->type != FS_NODE_DIRECTORY) {
		result = parent;
	}
	else if (len == 0) {
		result = parent;
	}
	else {
		current = parent->info.dir.child;
		while (current != NULL && strncmp(current->name, path, len)) {
			current = current->next_sibling;
		}
		if (current != NULL) {
			result = find_node(path + len, current);
		}
	}
	return result;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {//стандартный метод, получаем содержимое директории
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
	}
	else {
		printf("node not found!\n");
	}
	return 0;
}

static int do_getattr(const char *path, struct stat *st) {//стандартный метод, получаем информацию об объекте
	fs_node_t *node;
	int ret = 0;
	printf("do_getattr: %s\n", path);
	node = find_node(path, root);
	if (node != NULL) {
		st->st_mode = node->mode;
		if (node->type == FS_NODE_DIRECTORY) {
			st->st_mode |= S_IFDIR;//константа битовой маски каталога
		}
		else if (node->type == FS_NODE_FILE) {
			st->st_mode |= S_IFREG;//регулярное
			st->st_size = node->info.file.size;
		}
		st->st_uid = node->uid;
		st->st_gid = node->gid;
	}
	else {
		printf("node not found!");
		ret = -ENOENT;//нет такого файла или каталога
	}
	return ret;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {//чтение файла
	fs_node_t *node;
	int bytes_read = 0;
	node = find_node(path, root);//находит узел
	if (node != NULL && offset < node->info.file.size) {
		printf("read %s\n", node->name);
		if (!strcmp(node->name, "env")) {
			FILE *fileptr;
			fileptr = fopen("/foo/baz/evn", "rb");
			fseek(fileptr, 0, SEEK_END);                 //перемещает указатель позиции в потоке от начала до конца файла
			long fsize = ftell(fileptr);//возвращает количество байт от начала файла
			fseek(fileptr, 0, SEEK_SET);
			env = (char *)malloc(fsize);//выделяет блок памяти, указанного размера
			fread(env, fsize, 1, fileptr);
			fclose(fileptr);
			if (fsize - offset > size) {
				bytes_read = size;
				memcpy(buffer, env + offset, size);//копирует байты
			}
			else {
				bytes_read = fsize - offset;
				memcpy(buffer, env + offset, fsize - offset);
			}
		}
		else {
			int bytes_available = node->info.file.size - offset;
			if (bytes_available >= size) {
				bytes_read = size;
			}
			else {
				bytes_read = bytes_available;
			}
			if (bytes_read > 0) {
				memcpy(buffer, node->info.file.data_ptr, bytes_read);
			}
		}
	}
	return bytes_read;
}

static int do_mkdir(char *path, mode_t mode) {//  команда для созданния новых каталогов 
	char *pathRoot = (char*)malloc(100);
	char *name = (char*)malloc(100);
	for (int i = strlen(path) - 1; i >= 0; i--)
	{
		if (path[i] == '/')
		{
			strncpy(pathRoot, path, i);
			printf("\npath = %s \n", pathRoot);
			for (int ii = i + 1, j = 0; ii < strlen(path); ii++, j++)
			{
				name[j] = path[ii];
				//name = (char*)malloc(j);
			}
			printf("name = %s \n", name);
			break;
		}
	}
	fs_node_t *node = find_node(pathRoot, root);
	if (node != NULL) {
		add_directory(node, name, 0777);
		return 0;
	}
	return -1;
}

static struct fuse_operations operations = {
		.getattr = do_getattr,
		.readdir = do_readdir,
		.read = do_read,
		.mkdir = do_mkdir,
};

static void generate_tree() {
	fs_node_t *bar, *bin, *env, *readme, *foo, *test, *baz, *example;
	foo = add_directory(root, "foo", 0555);
	bin = add_directory(root, "bin", 0755);
	bar = add_directory(bin, "bar", 0777);
	baz = add_directory(foo, "baz", 0444);
	env = add_file(baz, "env", 0767);
	readme = add_file(baz, "readme.txt", 0426);
	test = add_file(baz, "test.txt", 0665);
	example = add_file(baz, "example", 0277);
	readme->info.file.data_ptr = "Student Наталия Струкова, 16150029\n\0";
	readme->info.file.size = strlen(readme->info.file.data_ptr);

	for (int i = 0; i < 15; i++) {
		Text[i * 2] = '_';
		Text[i * 2 + 1] = '\n';
	}
	Text[29] = '\0';
	test->info.file.data_ptr = Text;
	test->info.file.size = strlen(Text);
	example->info.file.data_ptr = "Hello world\n\0";
	example->info.file.size = strlen(example->info.file.data_ptr);
	FILE *fileptr;
	fileptr = fopen("/usr/bin/env", "rb");
	fseek(fileptr, 0, SEEK_END);
	long fsize = ftell(fileptr);
	fseek(fileptr, 0, SEEK_SET);
	env->info.file.size = fsize;
}

int main(int argc, char *argv[]) {
	root = create_directory("", 0666);
	generate_tree();

	return fuse_main(argc, argv, &operations, NULL);
}
