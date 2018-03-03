#define MULTI_LINE_STRING(a) #a
const char *include_text = MULTI_LINE_STRING(
		struct gtable{\n
		struct stack* gen_ptr;\n
		}gtb[50];\n
		\n
		struct stack{\n
		void* value;\n
		int type;\n
		int size;\n
		int num_mem;\n
		struct stack* link;\n
		};\n
		\n
		struct ltable{\n
		struct stack obj;\n
		int gtb_index;\n
		int size;\n
		};\n
		\n
		struct new_added{\n
		int index;\n
			struct new_added* link;\n
		};\n
\n
void create(int maxval){\n
	for(int i=0;i<maxval;i++)\n
		gtb[i].gen_ptr=NULL;\n
}\n
\n
int add_node(int index,int type,int size,int num_mem,struct ltable *ltb){\n
	if(gtb[index].gen_ptr!=NULL && type==gtb[index].gen_ptr->type){\n
		ltb->obj=*(gtb[index].gen_ptr);\n
			ltb->gtb_index=index;\n
			ltb->size=size;\n
			ltb->obj.value=malloc(size);\n
			memcpy(ltb->obj.value,(gtb[index].gen_ptr)->value,size);\n
			return -1;\n
	}\n
	\n
		struct stack *newNode;\n
		newNode = (struct stack*)malloc(sizeof(struct stack));\n
		newNode->type=type;\n
		newNode->size=size;\n
		newNode->num_mem=num_mem;\n
		newNode->value=malloc(size);\n
		\n
		if(gtb[index].gen_ptr == NULL){\n
			newNode->link = NULL;\n
		}\n
		else{\n
			newNode->link = gtb[index].gen_ptr;\n
		}\n
	gtb[index].gen_ptr = newNode; \n
		return 1;\n
}\n
\n
void restore(struct ltable ltb[],int count){\n
	for(int i=0;i<count;i++){\n
		*(gtb[ltb[i].gtb_index].gen_ptr)=ltb[i].obj;\n
			printf("---%d---\n",ltb[i].size);\n
			memcpy((gtb[ltb[i].gtb_index].gen_ptr)->value,ltb[i].obj.value,ltb[i].size);\n
			free(ltb[i].obj.value);\n
	}\n
}\n

struct new_added* na_push(struct new_added *head,int index){\n
	struct new_added* tmp = (struct new_added*)malloc(sizeof(struct new_added));\n
		tmp->index = index;\n
		tmp->link = head;\n
		head = tmp;\n
		return head;\n
}\n
\n
void na_pop(struct new_added *head){\n
	struct new_added *tmp=head;\n
		while(tmp!=NULL){\n
			struct stack* tmp1 = gtb[tmp->index].gen_ptr;\n
				if (gtb[tmp->index].gen_ptr!=NULL){\n
					gtb[tmp->index].gen_ptr = gtb[tmp->index].gen_ptr->link;\n
						free(tmp1);\n
				}\n
			tmp=tmp->link;\n
		}\n
}\n
);
