#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTNAME_LONG 64
#define MAX_OBJS 200000

struct obj_info {
	int type;
	int obj;
	int uid;
	int refed;
	int nref;
	int ref[10];
	off_t offset;
	char name_ori[MAX_FONTNAME_LONG];
	char name_new[MAX_FONTNAME_LONG];
};

int mycmp(const void *a, const void *b)
{
	struct obj_info *aa = (struct obj_info *)a;
	struct obj_info *bb = (struct obj_info *)b;
	return aa->offset - bb->offset;
}


int main(int argc, char **argv)
{
	int i;
	int type, obj, nobj;
	int ref, nref;
	off_t off;
	
	
	struct obj_info *objs = (struct obj_info *)malloc(MAX_OBJS * sizeof(struct obj_info));
	if(!objs)
	{
		printf("malloc failed\n");
		return -1;
	}
	memset(objs, 0, MAX_OBJS * sizeof(struct obj_info));
	
	int fd = open(argv[1], O_RDONLY);
	if(fd<0)
	{
		printf("open %s error\n", argv[1]);
		return -1;
	}
	
	int fd_out = open(argv[2], O_RDWR | O_CREAT);
	if(fd_out<0)
	{
		printf("open %s error\n", argv[2]);
		return -1;
	}
	
	
	nobj = 0;
	while(1)
	{
		//read obj
		scanf("%d\n", &obj);
		if(!obj) //exit when obj == 0
			break;
		
		if(obj>nobj) nobj = obj;
		
		//printf("%d\n", obj);
		(objs+obj)->obj = obj;

		//read type
		scanf("%d\n", &type);
		(objs+obj)->type = type;

		//read offset
		scanf("%lld\n", &off);
		(objs+obj)->offset = off;

		//read refs
		scanf("%d ", &nref);
		(objs+obj)->nref = nref;
		for(i=0;i<nref;i++)
		{
			scanf("%d", &ref);
			//objid == 1
			if(i<10)
				(objs+obj)->ref[i] = ref;
			(objs+ref)->refed = obj;
			//printf("%d ", ref);
		}

		if(type== 1 || type ==2) //font type has two more line
		{
		
			//read offset
			scanf("%lld\n", &off);
			(objs+obj)->offset = off;
			//printf("\n%lld\n", off);
			
			//read font name
			gets((objs+obj)->name_ori);
			//printf("1%s\n", (objs+obj)->name_ori);

			//remove the "-Identity-H"
			if(strstr((objs+obj)->name_ori, "-Identity-H"))
			{
				(objs+obj)->name_ori[strlen((objs+obj)->name_ori) - 11] = 0;
			}
			//printf("2%s\n", (objs+obj)->name_ori);
			
			//check font name
			char buff[MAX_FONTNAME_LONG] = {0};
			lseek(fd, off, SEEK_SET);
			read(fd, buff, strlen((objs+obj)->name_ori));
			//printf("%s\n", buff);
			if(memcmp((objs+obj)->name_ori,buff,strlen(buff)))
			{
				printf("%s!=%s\n",(objs+obj)->name_ori, buff);
				break;
			}
		}
	}
	
	int id = 1;
	for(i=0;i<=nobj;i++)
	{
		if((objs+i)->obj)
		{
			if(!(objs+i)->uid)
			{
				if((objs+i)->refed) //refered by other obj
				{
					if(!(objs+(objs+i)->refed)->uid) //if the origin obj have no uid too
					{
						(objs+(objs+i)->refed)->uid = id;
						id++;
					}
					(objs+i)->uid = (objs+(objs+i)->refed)->uid;
					//printf("%d uid%d %d %d\n",(objs+i)->obj, (objs+i)->uid, (objs+i)->refed, (objs+(objs+i)->refed)->uid);
				}
				else
				{
					(objs+i)->uid = id;
					id++;
				}
			}
		}
	}
	
	
	qsort(objs, nobj+1, sizeof(struct obj_info), mycmp);

	i = 0;
	while(!(objs+i++)->obj);
	i--;
	
	//debug
	for(;i<=nobj;i++)
	{
		if((objs+i)->obj)
		{
			printf("T%d O%d U%d R%d F%lld %s\n",(objs+i)->type, (objs+i)->obj, (objs+i)->uid, (objs+i)->refed, (objs+i)->offset, (objs+i)->name_ori);
		}
	}
	

	return 0;

	i = 0;
	while(!(objs+i++)->obj);
	i--;
	
	lseek(fd, 0, SEEK_SET);
	char c;
	off_t pdf_off = 0;
	while(read(fd,&c,1)==1)
	{
		char fontname_new[MAX_FONTNAME_LONG] = {0};
		if(pdf_off == (objs+i)->offset)
		{
			//sprintf(fontname_new, "%s-%d", (objs+i)->name_ori, (objs+i)->uid);
			sprintf(fontname_new, "/FONT-%d", (objs+i)->uid);
			write(fd_out, fontname_new, strlen(fontname_new));
			pdf_off = lseek(fd, strlen((objs+i)->name_ori)-1, SEEK_CUR);
			printf("O%d off %d %llu\n",(objs+i)->obj,strlen((objs+i)->name_ori),pdf_off);
			i++;
		}
		else
		{
			write(fd_out, &c, 1);
			pdf_off++;
		}
	}
	
	
	free(objs);
	close(fd);
	return 0;
}