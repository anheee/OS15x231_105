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

static void syscall_handler (struct intr_frame *);//用于控制整个系统调用过程


struct file_fd         //实现了文件的fd可以成功映射到该文件
{
	int fd;            //记录file的fd的值
	struct file *fp;    //记录指向文件的指针
	struct list_elem elem;  //用于list
};

struct list file_fd_list;  //用来存储file_fd

int write(int fd,const char *buffer,unsigned int size);//向一个文件中写入
void exit(int status);//终止当前process
void halt();//停止当前操作系统运行
int exec(const char *cmd_line);//开始另一个process
int wait(int pid);//等待一个子进程的死亡
bool create(const char *file,unsigned int initial_size);
//创建一个file
int open(const char *file);//打开一个文件
int read(int fd,void *buffer,unsigned int size);//从一个文件中读
int filesize(int fd);//获取一个文件的大小
bool remove(const char *file);//删除一个文件
void seek(int fd,unsigned int position);//改变在文件中的位置
unsigned int tell(int fd);//报告在文件中的当前位置
void close(int fd);//关闭文件

bool addr_is_right(void *addr);//检查地址是否合法

//系统调用初始化
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&file_fd_list);
}

//该函数查看当前是哪一个syscall，然后调用相应的系统调用函数
static void
syscall_handler (struct intr_frame *f UNUSED) //读取出中断帧
{        //系统调用可能会从用户内存中读取或写入函数，
		//所以在每调用一个函数之前都要先检验堆栈顶指针指向的
		//地址值是否合法。
	if(!addr_is_right(f->esp)) //指向栈顶的指针
		
		exit(-1);//如果不合法则调用exit（）函数来退出。
	
	int syscall=*(int *)(f->esp);
	//如果地址合法才进行执行系统调用。

	if(syscall==SYS_WRITE) 
		//根据用户不同的调用号，进行不同的系统调用。
	{
		if(addr_is_right(f->esp+4)&&addr_is_right(f->esp+8)&&addr_is_right(f->esp+12))
	 //检查这三个地址空间值是否合法
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

//实现向一个文件写入
int write(int fd,const char *buffer,unsigned int size)
{
	if(!addr_is_right((void *)buffer))
		//如果当前buffer指向的地址值无效
		exit(-1);
	if(fd == 1)	
	{
		putbuf(buffer,size); //写入缓冲中，并返回实际写入的byte数
		return size;
	}

	struct thread *t = thread_current();
	int i = 0;
	while(i<t->num_file_open&&t->open_file[i]!=fd) //搜索open_file array
		i++;
	if(i==t->num_file_open)  //如果当前线程没有打开这个文件，返回-1
			return -1;


	if(!list_empty(&file_fd_list))  
//如果打开了这个文件，搜索file_td_list找到fd对应的file_pointer

  {
		struct list_elem *le;
		for (le = list_begin (&file_fd_list);le != list_end (&file_fd_list);le = list_next (le))
		{
			struct file_fd *ff = list_entry (le, struct file_fd, elem);
			struct file *f = ff->fp;
		  if(ff->fd == fd)
				return file_write(f,buffer,size);  //最后调用write函数
		}
	}
}
 
void exit(int status)   //终止process，并记录线程的退出状态
{
		struct thread *t = thread_current();  
		//设置指针t指向当前线程
		t->exit_status = status;  //设置退出状态
		int i = 0;
		while(i<t->num_file_open)  //关闭所有打开的文件
		{
				close(t->open_file[i]);
				i++;
		}
		thread_exit();	 //调用该函数退出线程
}

void halt()
{
	power_off();  //通过调用power_off来终止pintos.
}

//执行另一个进程
int exec(const char *cmd_line)
{	
	if(!addr_is_right((void *)cmd_line))  
		//同样对指针指向的地址的合法性进行检查
		exit(-1);
	return process_execute(cmd_line);    
	//通过调用process_execute()
}

int wait(int pid)
{
	//struct thread *t = get_thread(pid);
	//sema_down(&t->sema);
	return process_wait(pid);
}

//创建一个file
bool create(const char *file,unsigned int initial_size) 
//需要的参数是文件名以及文件的大小
{
	if(!addr_is_right((void *)file))
		exit(-1);
	return filesys_create(file,initial_size);  
	//通过调用filesys_create来创建一个文件
}

//从一个文件中读
int read(int fd,void *buffer,unsigned int size)//传入一个缓冲
{
	if(!addr_is_right((void *)buffer))
		exit(-1);
	if(fd == 0)
	//如果fd为0，用input_getc从键盘读入，然后return读到的大小
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
		i++;         //搜索到文件fd
	if(i==t->num_file_open)  
//如果最后t的值与所有当前打开的文件数的值相等，则没有成功搜索到fd文件
		return -1;


	if(!list_empty(&file_fd_list))
  {
		struct list_elem *le;
		for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
	   //在file_fd_list中找到fd对应的file_pointer
		{
			struct file_fd *ff = list_entry (le, struct file_fd, elem);
			if(ff->fd == fd)
				return file_read(ff->fp,buffer,size); //最后调用file_read函数实现读
		}
	}	
}

//打开一个文件
int open(const char *file)
{
	static int fd = 10; //使用一个全局变量确保每一个文件的fd不同
	if(!addr_is_right((void *)file))
		exit(-1);
	struct file *f = filesys_open(file); //打开文件，如果有错返回-1,没有错将返回文件的fd,以及文件的指针
	struct thread *t = thread_current();
	
	if(f == NULL)
		return -1;
	
	if(t->num_file_open<128)
	{
		struct file_fd *ff = malloc(sizeof(struct file_fd)); //为ff分配file_fd大小的空间
	    ff->fp = f; 
		ff->fd = fd;

  	int i = 0;
		while(i<=t->num_file_open)
		{        
				if(t->open_file[i] == 0)
				{
					t->open_file[i] = fd; //设置数组中的标志位即第i个文件为fd
					t->num_file_open++; //设置打开的文件数的值自增
					break;
				}
					i++;
		}
		list_push_back(&file_fd_list,&ff->elem);  //将新增记录压入file_fd_list中
		fd++;
		if(strcmp(file,t->name) == 0)
			file_deny_write(ff->fp);  //文件名和当前线程名不能相等
		return ff->fd;
	}
}

bool remove(const char *file)
{
	if(!addr_is_right((void *)file))
		exit(-1);
	return filesys_remove(file);
}

//返回文件大小
int filesize(int fd)
{
	if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			//搜索list来找到fd。如果fd不存在list中，则找不到。
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem);
				if(ff->fd == fd)
					return file_length(ff->fp);  //调用file_length返回文件大小 
			}
	}
	return -1;
}

//搜索list来寻找fd，如果fd对应文件处于open状态，则设置position
void seek(int fd,unsigned int position)
{
	if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem);
				if(ff->fd == fd)
					file_seek(ff->fp,position); //调用file_seek函数实现
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
					return file_tell(ff->fp);  //调用file_tell
			}
	}
	
	return -1; 
}


void close(int fd)
{
		struct thread *t = thread_current(); //指针t指向当前线程
		int i = 0;
		while(i<t->num_file_open && t->open_file[i]!=fd)
			i++;    //当i小于当前线程打开的文件数且i对应的文件不是当前要关闭的文件时，i自增。
		if(i == t->num_file_open)
			return;  //如果i已经遍历了所有线程当前打开的文件，说明fd对应的文件已经处于close状态。

		if(!list_empty(&file_fd_list))
  	{
			struct list_elem *le;
			for (le = list_begin (&file_fd_list); le != list_end (&file_fd_list);le = list_next (le))
			//在file_fd_list中查找到指向fd的file pointer，并赋给le
			{
				struct file_fd *ff = list_entry (le, struct file_fd, elem); //把当前文件信息赋值给ff
				if(ff->fd == fd)   
				{
					t->num_file_open--;  //修改num_file_open的值，表示有一个文件已关闭
					t->open_file[i] = 0;  //i是当前文件在堆栈中的一个编号，0表示其已关闭
					file_close(ff->fp);  //调用file_close实现文件的关闭
					list_remove(le);//将le从file_fd_list中删除
					return;
				}	
			}
		}
}

//用于检查当前指针指向的地址值是否合法，合法则返回ture.
bool addr_is_right(void *addr)
{
	if(addr == NULL)   //若地址为空，返回false。
		return false;
	else if(!is_user_vaddr(addr))  //借用系统中的函数is_user_vaddr进行检查是不是进程的地址空间
		return false;
	else if(pagedir_get_page (thread_current()->pagedir,addr) == NULL)
		return false;
	else
		return true;
}
