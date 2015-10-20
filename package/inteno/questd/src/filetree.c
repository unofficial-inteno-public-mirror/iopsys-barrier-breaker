#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <libubox/list.h>
#include <limits.h>

#include "filetree.h"

static int getFiles(bool includeFiles, struct file *head){
	int ret = 0;
	DIR *dirpointer = NULL;
	struct dirent *direntryp = NULL;
	dirpointer = opendir(head->path);
	puts("test00");
	if(dirpointer == NULL){
		printf("error could not open %s \n", head->path);
		return -1;
	}else{
		while((direntryp = readdir(dirpointer))){
			puts("test1");
			if(direntryp == NULL){
				puts("error opening file");
				continue;
			}
			if(strcmp(direntryp->d_name, ".") == 0 || strcmp(direntryp->d_name, "..") == 0) continue;
			puts("test2");
			struct file *nextnode;
			puts("test3");
			file_alloc(nextnode, direntryp->d_name);
			puts("test4");
			list_add(&(nextnode->list), &(head->list));
			puts("test5");
			if(direntryp->d_type == DT_DIR){
				nextnode->folder = true;
				ret |= getFiles(includeFiles, nextnode);
			}
		}
		if(closedir(dirpointer) != 0){
			printf("error closing file: %s\n", head->path);
			return -1;
		}
	}
	return ret;
}
void file_destroy(struct file *self){
	struct list_head *pos, *n;
	struct file *tmp;
	list_for_each_safe(pos, n, &self->list){
		tmp = list_entry(pos, struct file, list);
		if(tmp == NULL)continue;
		if(tmp->folder == true){
			file_destroy(tmp->subhead);
		}
		free(tmp->path);
		free(tmp);
	}
}

void file_alloc(struct file *self, char *pth){
	self = (struct file *) calloc(1, sizeof(struct file));
	self->path = (char *) calloc(1, PATH_MAX);
	self->path = strdup(pth);
	self->folder = false;
	INIT_LIST_HEAD(&self->list);
	self->subhead = NULL;
}

int main(int argc, char **argv){
	(void) argc;
	(void) argv;
	struct file *head;
	file_alloc(head, strdup("."));
	int res = getFiles(true, head);
	file_destroy(head);
	return res;
}
