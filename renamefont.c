#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTNAME_LONG 64
#define MAX_OBJS 200000


#define INFO0(fmt, args...)
#define INFO1(fmt, args...)
#define INFO2(fmt, args...) printf(fmt, ##args)

struct obj_info {
	int type;
	int obj;
	int uid;
	int refed;
	int nref;
	int ref[10];
	off_t offset;
	off_t start;
	off_t end;
	off_t length;
	char name_ori[MAX_FONTNAME_LONG];
	char name_new[MAX_FONTNAME_LONG];
	int compress_type;
};

int mycmp(const void *a, const void *b)
{
	struct obj_info *aa = (struct obj_info *)a;
	struct obj_info *bb = (struct obj_info *)b;
	if(aa->obj && bb->obj)
	{
		return aa->offset - bb->offset;
	}
	else
	{
		if(!aa->obj)
			return 1;
		else
			return -1;
	}
	
}


//read form stdin
int init_objects(struct obj_info *objs)
{
	int maxobj = 0;
	int type, obj;
	int i, ref, nref;
	
	while(1)
	{
		//read obj
		scanf("%d\n", &obj);
		if(!obj) //exit when obj == 0
			break;
		
		if(obj>maxobj) maxobj = obj;
		
		INFO0("obj:%d\n", obj);
		(objs+obj)->obj = obj;

		//read offset
		scanf("%lld\n", &(objs+obj)->offset);

		//read type
		scanf("%d\n", &(objs+obj)->type);
		INFO0("type:%d\n", (objs+obj)->type);
		
		//read refs
		scanf("%d ", &nref);
		(objs+obj)->nref = nref;
		INFO0("ref:(%d)",nref);
		for(i=0;i<nref;i++)
		{
			scanf("%d", &ref);
			if(i<10)
				(objs+obj)->ref[i] = ref;
			(objs+ref)->refed = obj;
			INFO0("%d ", ref);
		}
		INFO0("\n");

		if((objs+obj)->type == 0)
		{
			if(!(objs+obj)->nref) //should be the length of stream
			{
				scanf("%lld\n", &(objs+obj)->length);
				scanf("%lld %lld\n", &(objs+obj)->start, &(objs+obj)->end);
			}
			else
			{
				//should never be here
				printf("ERROR\n");
				return -1;
			}
		}
		else if((objs+obj)->type == 1 || (objs+obj)->type == 2)
		{
			//read start offset
			scanf("%lld\n", &(objs+obj)->start);
			
			//read font name
			gets((objs+obj)->name_ori);
			INFO0("%s\n", (objs+obj)->name_ori);

			//remove the "-Identity-H"
			if(strstr((objs+obj)->name_ori, "-Identity-H"))
			{
				(objs+obj)->name_ori[strlen((objs+obj)->name_ori) - 11] = 0;
			}
			(objs+obj)->end = (objs+obj)->start + strlen((objs+obj)->name_ori);
			
			//check font name
			#if 0
			char buff[MAX_FONTNAME_LONG] = {0};
			lseek(fd, off, SEEK_SET);
			read(fd, buff, strlen((objs+obj)->name_ori));
			if(memcmp((objs+obj)->name_ori,buff,strlen(buff)))
			{
				printf("%s!=%s\n",(objs+obj)->name_ori, buff);
				break;
			}
			#endif
		}
		else if((objs+obj)->type == 3) //stream
		{
			char buff[MAX_FONTNAME_LONG] = {0};
			if(!(objs+obj)->nref)//null stream
			{
				gets(buff);
				gets(buff);
				gets(buff);
				INFO0("NULL stream\n");
				(objs+obj)->obj = 0;
			}
			else
			{
				scanf("%d", &ref); //ref should be same with (objs+obj)->ref[0]
				gets(buff);
				scanf("%lld %lld\n", &(objs+obj)->start, &(objs+obj)->end);
				if(strstr(buff, "FlateDecode"))
				{
					(objs+obj)->compress_type = 1;
				}
			}
		}
		else
		{
			//should never be here
			printf("ERROR\n");
			return -1;
		}
	}
	
	return maxobj;
}

int update_objects(struct obj_info *objs, int maxobj)
{
	//UID
	int i,j,id = 1;
	for(i=0;i<=maxobj;i++)
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
					INFO1("O%d U%d R%d RD%d RDU%d\n",(objs+i)->obj, (objs+i)->uid, (objs+i)->ref[0], (objs+i)->refed, (objs+(objs+i)->refed)->uid);
					if((objs+i)->type == 3 && (objs+(objs+i)->refed)->name_ori[0])
					{
						memcpy((objs+i)->name_ori, (objs+(objs+i)->refed)->name_ori, strlen((objs+(objs+i)->refed)->name_ori));
					}
				}
				else
				{
					(objs+i)->uid = id;
					INFO1("O%d U%d R%d RD%d RDU%d\n",(objs+i)->obj, (objs+i)->uid, (objs+i)->ref[0], (objs+i)->refed, (objs+(objs+i)->refed)->uid);
					//update all ref obj
					for(j=0; j<(objs+i)->nref;j++)
					{
						(objs+(objs+i)->ref[j])->uid = id;
						if((objs+i)->name_ori[0] && !(objs+(objs+i)->ref[j])->name_ori[0])
						{
							memcpy((objs+(objs+i)->ref[j])->name_ori, (objs+i)->name_ori, strlen((objs+i)->name_ori));
						}
					}
					id++;
				}
			}
		}
	}
	
	
	qsort(objs, maxobj+1, sizeof(struct obj_info), mycmp);

	i=0;
	while((objs+i++)->obj);
	i--;
	
	//debug
	INFO2("total %d\n", i);
	for(j=0;j<i;j++)
		INFO2("O%d T%d (%lld\t%lld) U%d RD%d F%lld\n",(objs+j)->obj, (objs+j)->type, (objs+j)->start, (objs+j)->end, (objs+j)->uid, (objs+j)->refed, (objs+j)->offset);
	
	return i;
}

int uncompress(char *origin, char *new)
{
	int new_len = 0;
	return new_len;
}

int compress(char *origin, char *new)
{
	int new_len = 0;
	return new_len;
}

int update_obj_length(struct obj_info *objs, int fd_in, int maxobj)
{
	//UID
	int i,j,id = 1;
	for(i=0;i<maxobj;i++)
	{
		if((objs+i)->type == 3)
		{
			int ml = (objs+i)->end - (objs+i)->start;
			char *ori_stream = (char *)malloc(ml+1);
			char *new_stream = (char *)malloc(ml*2+1);
			if(!ori_stream || !new_stream)
			{
				printf("malloc failed\n");
				return -1;
			}
			memset(ori_stream, 0, ml+1);
			memset(new_stream, 0, ml*2+1);
			
			int off = 0;
			lseek(fd_in, (objs+i)->start, SEEK_SET);
			while(off < ml && read(fd_in, ori_stream+off, 1)) off++;
			
			uncompress(ori_stream, new_stream);
			
			//TODO: replace the font name in new_stream
			//
			
			//TODO open>>memset(ori_stream, 0, ml+1);
			//update ref length
			//TODO open>>(objs+(objs+i)->ref[0])->length = compress(new_stream, ori_stream);
			
			#if 1
			char filename[32] = {0};
			sprintf(filename, "stream/%d-%d-%s", (objs+i)->obj, ml, (objs+i)->name_ori);
			int out = open(filename, O_RDWR | O_CREAT);
			if(out<0)
			{
				printf("open %s error\n", filename);
				return -1;
			}
			off = 0;
			while(off < ml && write(out, ori_stream+off, 1)) off++;

			close(out);
			#endif
			
			free(ori_stream);
			free(new_stream);
		}
	}
}

int main(int argc, char **argv)
{
	int i;
	int maxobj = 0;
	
	struct obj_info *objs = (struct obj_info *)malloc(MAX_OBJS * sizeof(struct obj_info));
	if(!objs)
	{
		printf("malloc failed\n");
		return -1;
	}
	memset(objs, 0, MAX_OBJS * sizeof(struct obj_info));
	
	int fd_in = open(argv[1], O_RDONLY);
	if(fd_in<0)
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
	
	maxobj = init_objects(objs);
	
	maxobj = update_objects(objs, maxobj);
	
	update_obj_length(objs, fd_in, maxobj);
	
	return 0;
	
	lseek(fd_in, 0, SEEK_SET);
	char c;
	off_t pdf_off = 0;
	i = 0;
	while(read(fd_in,&c,1)==1)
	{
		char fontname_new[MAX_FONTNAME_LONG] = {0};
		if(pdf_off == (objs+i)->start)
		{
			if((objs+i)->type == 0)
			{
				sprintf(fontname_new, "%d", (objs+i)->length);
				write(fd_out, fontname_new, strlen(fontname_new));
				pdf_off = lseek(fd_in, (objs+i)->end, SEEK_SET);
			}
			else if((objs+i)->type == 1 || (objs+i)->type == 2)
			{
				sprintf(fontname_new, "FONT-%d", (objs+i)->uid);
				write(fd_out, fontname_new, strlen(fontname_new));
				pdf_off = lseek(fd_in, (objs+i)->end, SEEK_SET);
				INFO0("O%d off %d %llu\n",(objs+i)->i,strlen((objs+i)->name_ori),pdf_off);
			}
			else if((objs+i)->type == 3)
			{
				
			}
			i++;
		}
		else
		{
			write(fd_out, &c, 1);
			pdf_off++;
		}
	}
	
	
	free(objs);
	close(fd_in);
	close(fd_out);
	return 0;
}