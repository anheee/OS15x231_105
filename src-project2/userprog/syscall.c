#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "lib/kernel/stdio.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);//���ڿ�������ϵͳ���ù���


struct file_fd         //ʵ�����ļ���fd���Գɹ�ӳ�䵽���ļ�
{
	int fd;            //��¼file��fd��ֵ
	struct file *fp;    //��¼ָ���ļ���ָ��
	struct list_elem elem;  //����list
};

struct list file_fd_list;  //�����洢file_fd

int write(int fd,const char *buffer,unsigned int size);//��һ���ļ���д��
void exit(int status);//��ֹ��ǰprocess
void halt();//ֹͣ��ǰ����ϵͳ����
int exec(const char *cmd_line);//��ʼ��һ��process
int wait(int pid);//�ȴ�һ���ӽ��̵�����
bool create(const char *file,unsigned int initial_size);
//����һ��file
int open(const char *file);//��һ���ļ�
int read(int fd,void *buffer,unsigned int size);//��һ���ļ��ж�
int filesize(int fd);//��ȡһ���ļ��Ĵ�С
bool remove(const char *file);//ɾ��һ���ļ�
void seek(int fd,unsigned int position);//�ı����ļ��е�λ��
unsigned int tell(int fd);//�������ļ��еĵ�ǰλ��
void close(int fd);//�ر��ļ�

bool addr_is_right(void *addr);//����ַ�Ƿ�Ϸ�

//ϵͳ���ó�ʼ��
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&file_fd_list);
}

//�ú����鿴��ǰ����һ��syscall��Ȼ�������Ӧ��ϵͳ���ú���
static void
syscall_handler (struct intr_frame *f UNUSED) //��ȡ���ж�֡
{        //ϵͳ���ÿ��ܻ���û��ڴ��ж�ȡ��д�뺯����
		//������ÿ����һ������֮ǰ��Ҫ�ȼ����ջ��ָ��ָ���
		//��ֵַ�Ƿ�Ϸ���
	if(!addr_is_right(f->esp)) //ָ��ջ����ָ��
		
		exit(-1);//������Ϸ������exit�����������˳���
	
	int syscall=*(int *)(f->esp);
	//�����ַ�Ϸ��Ž���ִ��ϵͳ���á�

	if(syscall==SYS_WRITE) 
		//�����û���ͬ�ĵ��úţ����в�ͬ��ϵͳ���á�
	{
		if(addr_is_right(f->esp+4)&&addr_is_right(f->esp+8)&&addr_is_right(f->esp+12))
	 //�����������ַ�ռ�ֵ�Ƿ�Ϸ�
		{
			int fd = *(int *)(f->esp+4); 
			const char *buffer = (char *)*(unsigned int *)(f->esp+8);
			unsigned int size = *(unsigned int*)(f->esp+12);
			f->eax = write(fd,buffer,size);
		}
		else
				exit(-1);
	}

	if(syscall==SYS_EXIT)
	{
		if(addr_is_right(f->esp+4))
		{
			int status = *(int *)(f->esp+4);
			exit(status);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_HALT)
		halt();

	if(syscall==SYS_EXEC)
	{
		if(addr_is_right(f->esp+4))
		{
			const char *cmd_line = (char *)*(unsigned int*)(f->esp+4);
			f->eax = exec(cmd_line);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_WAIT)
	{
		if(addr_is_right(f->esp+4))
		{
			int pid = *(int *)(f->esp+4);
			f->eax = wait(pid);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_CREATE)
	{
		if(addr_is_right(f->esp+4)&&addr_is_right(f->esp+8))
		{
			const char *file = (char *)*(unsigned int*)(f->esp+4);
			unsigned int size = *(unsigned int*)(f->esp+8);
			f->eax = create(file,size);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_REMOVE)
	{
		if(addr_is_right(f->esp+4))
		{
			const char *file = (char *)*(unsigned int*)(f->esp+4);
			f->eax = remove(file);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_OPEN)
	{
		if(addr_is_right(f->esp+4))
		{
			const char *file = (char *)*(unsigned int*)(f->esp+4);
			f->eax = open(file);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_FILESIZE)
	{
		if(addr_is_right(f->esp+4))
		{
			int fd = *(int *)(f->esp+4);
			f->eax = filesize(fd);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_READ)
	{
		if(addr_is_right(f->esp+4)&&addr_is_right(f->esp+8)&&addr_is_right(f->esp+12))
		{
			int fd = *(int *)(f->esp+4);
			const char *buffer = (char *)*(unsigned int *)(f->esp+8);
			unsigned int size = *(unsigned int*)(f->esp+12);
			f->eax = read(fd,buffer,size);
		}
		else
			exit(-1);
	}	

	if(syscall==SYS_SEEK)
	{
		if(addr_is_right(f->esp+4)&&addr_is_right(f->esp+8))
		{
			int fd = *(int *)(f->esp+4);
			unsigned int position = *(unsigned int *)(f->esp+8);
			seek(fd,position);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_TELL)
	{
		if(addr_is_right(f->esp+4))
		{
			int fd = *(int *)(f->esp+4);
			f->eax = tell(fd);
		}
		else
			exit(-1);
	}

	if(syscall==SYS_CLOSE)
	{
		if(addr_is_right(f->esp+4))
		{
			int fd = *(int *)(f->esp+4);
			close(fd);
		}
		else
			exit(-1);
	}
}

//ʵ����һ���ļ�д��
int write(int fd,const char *buffer,unsigned int size)
{
	if(!addr_is_right((void *)buffer))
		//�����ǰbufferָ��ĵ�ֵַ��Ч
		exit(-1);
	if(fd == 1)	
	{
		putbuf(buffer,size); //д�뻺���У�������ʵ��д���byte��
		return size;
	}

	struct thread *t = thread_current();
	int i = 0;
	while(i<t->num_file_open&&t->open_file[i]!=fd) //����open_file array
		i++;
	if(i==t->num_file_open)  //�����ǰ�߳�û�д�����ļ�������-1
			return -1;


	if(!list_empty(&file_fd_list))  
//�����������ļ�������file_td_list�ҵ�fd��Ӧ��file_pointer

  {
		struct list_elem *le;
		for (le = list_begin (&file_fd_list);le != list_end (&file_fd_list);le = list_next (le))
		{
			struct file_fd *ff = list_entry (le, struct file_fd, elem);
			struct file *f = ff->fp;
		  if(ff->fd == fd)
				return file_write(f,buffer,size);  //������write����
		}
	}
}
 
void exit(int status)   //��ֹprocess������¼�̵߳��˳�״̬
{
		struct thread *t = thread_current();  
		//����ָ��tָ��ǰ�߳�
		t->exit_status = status;  //�����˳�״̬
		int i = 0;
		while(i<t->num_file_open)  //�ر����д򿪵��ļ�
		{
				close(t->open_file[i]);
				i++;
		}
		thread_exit();	 //���øú����˳��߳�
}

void halt()
{
	power_off();  //ͨ������power_off����ֹpintos.
}

//ִ����һ������
int exec(const char *cmd_line)
{	
	if(!addr_is_right((void *)cmd_line))  
		//ͬ����ָ��ָ��ĵ�ַ�ĺϷ��Խ��м��
		exit(-1);
	return process_execute(cmd_line);    
	//ͨ������process_execute()
}

int wait(int pid)
{
	//struct thread *t = get_thread(pid);
	//sema_down(&t->sema);
	return process_wait(pid);
}

//����һ��file
bool create(const char *file,unsigned int initial_size) 
//��Ҫ�Ĳ������ļ����Լ��ļ��Ĵ�С
{
	if(!addr_is_right((void *)file))
		exit(-1);
	return filesys_create(file,initial_size);  
	//ͨ������filesys_create������һ���ļ�
}

//��һ���ļ��ж�
int read(int fd,void *buffer,unsigned int size)//����һ������
{
	if(!addr_is_right((void *)buffer))
		exit(-1);
	if(fd == 0)
	//���fdΪ0����input_getc�Ӽ��̶��룬Ȼ��return�����Ĵ�С
	{
		int m = 0;
		for(m = 0 ; m < size ; m++)
		{
			*(char *)buffer = input_getc();
			buffer++;
		}
		return size;
	}
	struct thread *t = thread_current();
	int i = 0;
	while(i< t->num_file_open&&t->open_file[i]!= fd)
		i++;         //�������ļ�fd
	if(i==t->num_file_open)  
//������t��ֵ�����е�ǰ�򿪵��ļ�����ֵ��ȣ���û�гɹ�������fd�ļ�
		return -1;


	if(!list_empty(&file_fd_list))
  {
		struct list_elem *le;
		for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
	   //��file_fd_list���ҵ�fd��Ӧ��file_pointer
		{
			struct file_fd *ff = list_entry (le, struct file_fd, elem);
			if(ff->fd == fd)
				return file_read(ff->fp,buffer,size); //������file_read����ʵ�ֶ�
		}
	}	
}

//��һ���ļ�
int open(const char *file)
{
	static int fd = 10; //ʹ��һ��ȫ�ֱ���ȷ��ÿһ���ļ���fd��ͬ
	if(!addr_is_right((void *)file))
		exit(-1);
	struct file *f = filesys_open(file); //���ļ�������д���-1,û�д������ļ���fd,�Լ��ļ���ָ��
	struct thread *t = thread_current();
	
	if(f == NULL)
		return -1;
	
	if(t->num_file_open<128)
	{
		struct file_fd *ff = malloc(sizeof(struct file_fd)); //Ϊff����file_fd��С�Ŀռ�
	    ff->fp = f; 
		ff->fd = fd;

  	int i = 0;
		while(i<=t->num_file_open)
		{        
				if(t->open_file[i] == 0)
				{
					t->open_file[i] = fd; //���������еı�־λ����i���ļ�Ϊfd
					t->num_file_open++; //���ô򿪵��ļ�����ֵ����
					break;
				}
					i++;
		}
		list_push_back(&file_fd_list,&ff->elem);  //��������¼ѹ��file_fd_list��
		fd++;
		if(strcmp(file,t->name) == 0)
			file_deny_write(ff->fp);  //�ļ����͵�ǰ�߳����������
		return ff->fd;
	}
}

bool remove(const char *file)
{
	if(!addr_is_right((void *)file))
		exit(-1);
	return filesys_remove(file);
}

//�����ļ���С
int filesize(int fd)
{
	if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			//����list���ҵ�fd�����fd������list�У����Ҳ�����
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem);
				if(ff->fd == fd)
					return file_length(ff->fp);  //����file_length�����ļ���С 
			}
	}
	return -1;
}

//����list��Ѱ��fd�����fd��Ӧ�ļ�����open״̬��������position
void seek(int fd,unsigned int position)
{
	if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem);
				if(ff->fd == fd)
					file_seek(ff->fp,position); //����file_seek����ʵ��
			}
	}
	
	return ; 
}


unsigned int tell(int fd)
{

	if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem);
				if(ff->fd == fd)
					return file_tell(ff->fp);  //����file_tell
			}
	}
	
	return -1; 
}


void close(int fd)
{
		struct thread *t = thread_current(); //ָ��tָ��ǰ�߳�
		int i = 0;
		while(i<t->num_file_open && t->open_file[i]!=fd)
			i++;    //��iС�ڵ�ǰ�̴߳򿪵��ļ�����i��Ӧ���ļ����ǵ�ǰҪ�رյ��ļ�ʱ��i������
		if(i == t->num_file_open)
			return;  //���i�Ѿ������������̵߳�ǰ�򿪵��ļ���˵��fd��Ӧ���ļ��Ѿ�����close״̬��

		if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			//��file_fd_list�в��ҵ�ָ��fd��file pointer��������le
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem); //�ѵ�ǰ�ļ���Ϣ��ֵ��ff
				if(ff->fd == fd)   
				{
					t->num_file_open--;  //�޸�num_file_open��ֵ����ʾ��һ���ļ��ѹر�
					t->open_file[i] = 0;  //i�ǵ�ǰ�ļ��ڶ�ջ�е�һ����ţ�0��ʾ���ѹر�
					file_close(ff->fp);  //����file_closeʵ���ļ��Ĺر�
					list_remove(le);//��le��file_fd_list��ɾ��
					return;
				}	
			}
		}
}

//���ڼ�鵱ǰָ��ָ��ĵ�ֵַ�Ƿ�Ϸ����Ϸ��򷵻�ture.
bool addr_is_right(void *addr)
{
	if(addr == NULL)   //����ַΪ�գ�����false��
		return false;
	else if(!is_user_vaddr(addr))  //����ϵͳ�еĺ���is_user_vaddr���м���ǲ��ǽ��̵ĵ�ַ�ռ�
		return false;
	else if(pagedir_get_page (thread_current()->pagedir,addr) == NULL)
		return false;
	else
		return true;
}
