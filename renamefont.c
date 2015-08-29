#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#define MAX_FONTNAME_LONG 128
#define MAX_OBJS 200000

#define DEBUG_DUMP

#define ERROR(fmt, args...) printf(fmt, ##args)
#define INFO0(fmt, args...) 
#define INFO1(fmt, args...)
#define INFO2(fmt, args...)
#define INFO3(fmt, args...) printf(fmt, ##args)


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
	int CIDFontType0C;
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
				scanf("%d\n", &ref); //ref should be same with (objs+obj)->ref[0]
				gets(buff);
				if(strstr(buff, "FlateDecode"))
				{
					(objs+obj)->compress_type = 1;
				}
				memset(buff, 0, MAX_FONTNAME_LONG);
				gets(buff);
				if(!memcmp(buff, "CIDFontType0C", strlen("CIDFontType0C")))
				{
					(objs+obj)->CIDFontType0C = 1;
					scanf("%lld %lld\n", &(objs+obj)->start, &(objs+obj)->end);
				}
				else
				{
					sscanf(buff,"%lld %lld\n", &(objs+obj)->start, &(objs+obj)->end);
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

//1. UID
//2. fontname for type3
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
	
	return 0;
}

//memory maybe contain '\0'
int replace_string(char *sSrc, char *sMatchStr, char *sReplaceStr, int *length, int flag)
{
        int  StringLen;
        
		
		char *caNewString = malloc(*length);
		if(!caNewString)
		{
			return -1;
		}
		memset(caNewString, 0, *length);
		
		int k,j,i = 0;
		int len = 0;
        while(i < *length)
        {
                if(sSrc[i] == sMatchStr[0])
				{
					j = i+1;
					k=1;
					while(sSrc[j] == sMatchStr[k] && k<strlen(sMatchStr))
					{
						j++;
						k++;
					}
					if(k==strlen(sMatchStr) && flag)
					{
						flag--;
						i+=strlen(sMatchStr);
						memcpy(caNewString+len, sReplaceStr, strlen(sReplaceStr));
						len+=strlen(sReplaceStr);
						continue;
					}
				}
                caNewString[len++] = sSrc[i];
				i++;
        }
		
		memset(sSrc,0, *length);
		*length = len;
		memcpy(sSrc, caNewString, *length);
		free(caNewString);
        return 0;
}

int update_stream(struct obj_info *obj, char *ori_stream, unsigned long *ml)
{
	if(obj->name_ori[0] == 0)
	{
		printf("O%d return\n", obj->obj);
		return 0;
	}
	
	int ret;
	off_t off;
	unsigned long new_length = *ml*10;
	
	char *new_stream = (char *)malloc(new_length);
	if(!new_stream)
	{
		ERROR("malloc failed %d\n", new_length);
		return -1;
	}
	memset(new_stream, 0, new_length);
	
#ifdef DEBUG_DUMP
	char filename1[128] = {0};
	sprintf(filename1, "stream/%d-%d-%d-%s-origin", obj->obj, *ml, obj->start, obj->name_ori[0]?obj->name_ori:"null");
	int out1 = open(filename1, O_RDWR | O_CREAT);
	if(out1<0)
	{
		printf("open %s error\n", filename1);
	}
	else
	{
		off = 0;
		while(off < *ml && write(out1, ori_stream+off, 1)) off++;
		close(out1);
	}
#endif

	ret = uncompress(new_stream, &new_length, ori_stream, *ml+1);
	if(ret != Z_OK)  
	{  
		ERROR("uncompress failed (%d) !\n", ret);  
		return -1;  
	} 
#ifdef DEBUG_DUMP
	char filename2[128] = {0};
	sprintf(filename2, "stream/%d-%d-%d-%s-origin-uncompress", obj->obj, *ml, obj->start, obj->name_ori[0]?obj->name_ori:"null");
	int out2 = open(filename2, O_RDWR | O_CREAT);
	if(out2<0)
	{
		printf("open %s error\n", filename2);
	}
	else
	{
		off = 0;
		while(off < new_length && write(out2, new_stream+off, 1)) off++;
		close(out2);
	}
#endif

	//replace the fontname 
	INFO3("O%d (%s) -> (%s)\n", obj->obj, obj->name_ori, obj->name_new);
	replace_string(new_stream, obj->name_ori, obj->name_new, &new_length, 99);

#ifdef DEBUG_DUMP
	char filename3[128] = {0};
	sprintf(filename3, "stream/%d-%d-%d-%s-new-uncompress", obj->obj, *ml, obj->start, obj->name_ori[0]?obj->name_ori:"null");
	int out3 = open(filename3, O_RDWR | O_CREAT);
	if(out3<0)
	{
		printf("open %s error\n", filename3);
	}
	else
	{
		off = 0;
		while(off < new_length && write(out3, new_stream+off, 1)) off++;
		close(out3);
	}
#endif

	//compress data to orgin stream buf
	unsigned long tl = *ml*2;
	memset(ori_stream, 0, tl);
	ret = compress(ori_stream, &tl, new_stream, new_length);
	if(ret != Z_OK)  
	{  
		ERROR("compress failed (%d) !\n", ret);  
		return -1;  
	}
	*ml = tl;
	
#ifdef DEBUG_DUMP
	char filename4[128] = {0};
	sprintf(filename4, "stream/%d-%d-%d-%s-new-compress", obj->obj, *ml, obj->start, obj->name_ori[0]?obj->name_ori:"null");
	int out4 = open(filename4, O_RDWR | O_CREAT);
	if(out4<0)
	{
		printf("open %s error\n", filename4);
	}
	else
	{
		off = 0;
		while(off < *ml && write(out4, ori_stream+off, 1)) off++;
		close(out4);
	}
#endif
	
	free(new_stream);
	
	return 0;
}

int update_fontname(struct obj_info *objs, int maxobj)
{
	int i,j,id = 1;
	for(i=0;i<=maxobj;i++)
	{
		if((objs+i)->obj && (objs+i)->name_ori[0])
		{
			char buf[64] = {0};
			sprintf(buf, "%04d", (objs+i)->uid);
			memcpy((objs+i)->name_new, (objs+i)->name_ori, strlen((objs+i)->name_ori)-4);
			memcpy((objs+i)->name_new + strlen((objs+i)->name_ori)-4, buf,4);
		}
	}
}

int update_obj_length(struct obj_info *objs, int fd_in, int maxobj)
{
	int i,j,id = 1;
	for(i=0;i<=maxobj;i++)
	{
		if((objs+i)->obj && (objs+i)->type == 3 && (objs+i)->compress_type == 1)
		{
			unsigned long ml = (objs+i)->end - (objs+i)->start;
			char *ori_stream = (char *)malloc(ml*2);
			if(!ori_stream)
			{
				ERROR("malloc failed %d\n", ml);
				return -1;
			}
			memset(ori_stream, 0, ml*2);
			
			int off = 0;
			lseek(fd_in, (objs+i)->start, SEEK_SET);
			while(off < ml && read(fd_in, ori_stream+off, 1)) off++;
			
			update_stream(objs+i, ori_stream, &ml);

			//update ref length
			(objs+(objs+i)->ref[0])->length = ml;
			INFO2("change O%d %d\n", (objs+i)->ref[0], (objs+(objs+i)->ref[0])->length);

			free(ori_stream);
		}
	}
}

int main(int argc, char **argv)
{
	int i,j;
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
	
	update_objects(objs, maxobj);
	
	update_fontname(objs, maxobj);
	
	update_obj_length(objs, fd_in, maxobj);

	//sort the objects by offset
	qsort(objs, maxobj+1, sizeof(struct obj_info), mycmp);

	//debug
	i=0;
	while((objs+i++)->obj);
	i--;
	INFO2("total %d\n", i);
	for(j=0;j<i;j++)
		INFO2("O%d T%d (%lld\t%lld) U%d RD%d F%lld\n",(objs+j)->obj, (objs+j)->type, (objs+j)->start, (objs+j)->end, (objs+j)->uid, (objs+j)->refed, (objs+j)->offset);	
	
	
	lseek(fd_in, 0, SEEK_SET);
	char c;
	off_t pdf_off = 0;
	i = 0;
	while(read(fd_in,&c,1)==1)
	{
		if(pdf_off == (objs+i)->start)
		{
			if((objs+i)->type == 0 && (objs+i)->nref == 0)
			{
				char length_string[MAX_FONTNAME_LONG] = {0};
				INFO2("O%d %d\n", (objs+i)->obj, (objs+i)->length);
				sprintf(length_string, "%d", (objs+i)->length);
				write(fd_out, length_string, strlen(length_string));
				pdf_off = lseek(fd_in, (objs+i)->end, SEEK_SET);
			}
			else if((objs+i)->type == 1 || (objs+i)->type == 2)
			{
				write(fd_out, (objs+i)->name_new, strlen((objs+i)->name_new));
				pdf_off = lseek(fd_in, (objs+i)->end, SEEK_SET);
				INFO0("O%d off %d %llu\n",(objs+i)->obj,strlen((objs+i)->name_ori), pdf_off);
			}
			else if((objs+i)->type == 3)
			{
				unsigned long ml = (objs+i)->end - (objs+i)->start;
				char *ori_stream = (char *)malloc(ml*2);
				if(!ori_stream)
				{
					ERROR("malloc failed %lu\n", ml);
					return -1;
				}
				memset(ori_stream, 0, ml*2);
				
				int off = 0;
				lseek(fd_in, (objs+i)->start, SEEK_SET);
				while(off < ml && read(fd_in, ori_stream+off, 1)) off++;
				
				update_stream(objs+i, ori_stream, &ml);

				write(fd_out, ori_stream, ml);
				free(ori_stream);
				
				pdf_off = lseek(fd_in, (objs+i)->end, SEEK_SET);
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