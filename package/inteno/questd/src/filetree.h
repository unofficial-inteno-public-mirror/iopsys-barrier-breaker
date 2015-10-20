#ifndef HEADDER_FILETREE
#define HEADER_FILETREE

struct file{
	char *path;
	bool folder;
	struct list_head list;
	struct file *subhead;
};

void file_destroy(struct file *);
void file_alloc(struct file *, char *);

#endif

