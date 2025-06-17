/*
 * It opens an eventpoll file descriptor by allocating space for "maxfds"
 * file descriptors
 * kernel part of the userspace epoll_create(2)
 */
asmlinkage int sys_epoll_create(int maxfds)
{
  int error = -EINVAL, fd;
  unsigned long addr;
  struct inode *inode;
  struct file *file;
  struct eventpoll *ep;
​
  /*
   * eventpoll接口中不可能存储超过MAX_FDS_IN_EVENTPOLL的fd
   */
  if (maxfds > MAX_FDS_IN_EVENTPOLL)
    goto eexit_1;
​
  /*
   * Creates all the items needed to setup an eventpoll file. That is,
   * a file structure, and inode and a free file descriptor.
   */
  error = ep_getfd(&fd, &inode, &file);
  if (error)
    goto eexit_1;
​
  /*
   * 调用去初始化eventpoll file. 这和"open" file operation callback一样，因为 inside
   * ep_getfd() we did what the kernel usually does before invoking
   * corresponding file "open" callback.
   */
  error = open_eventpoll(inode, file);
  if (error)
    goto eexit_2;
​
  /* "private_data" 由open_eventpoll()设置 */
  ep = file->private_data;
​
  /* 分配页给event double buffer */
  error = ep_do_alloc_pages(ep, EP_FDS_PAGES(maxfds + 1));
  if (error)
    goto eexit_2;
​
  /*
   * 创建event double buffer的一个用户空间的映射，以避免当返回events给调用者时，内核到用户空间的内存复制
   */
  down_write(&current->mm->mmap_sem);
  addr = do_mmap_pgoff(file, 0, EP_MAP_SIZE(maxfds + 1), PROT_READ,
           MAP_PRIVATE, 0);
  up_write(&current->mm->mmap_sem);
  error = PTR_ERR((void *) addr);
  if (IS_ERR((void *) addr))
    goto eexit_2;
​
  return fd;
​
eexit_2:
  sys_close(fd);
eexit_1:
  return error;
}
asmlinkage int sys_epoll_ctl(int epfd, int op, int fd, unsigned int events)
{
  int error = -EBADF;
  struct file *file;
  struct eventpoll *ep;
  struct epitem *dpi;
  struct pollfd pfd;
​
  // 获取epfd对应的file实例
  file = fget(epfd);
  if (!file)
    goto eexit_1;
​
  /*
   * We have to check that the file structure underneath the file descriptor
   * the user passed to us _is_ an eventpoll file.
   * 检查fd对应文件是否是一个eventpoll文件
   */
  error = -EINVAL;
  if (!IS_FILE_EPOLL(file))
    goto eexit_2;
​
  /*
   * At this point it is safe to assume that the "private_data" contains
   * our own data structure.
   * 获取eventpoll文件中的私有数据，该数据是在epoll_create中创建的
   */
  ep = file->private_data;
​
  down_write(&ep->acsem);
​
  pfd.fd = fd;
  pfd.events = events | POLLERR | POLLHUP;
  pfd.revents = 0;
​
  // 在eventpoll中存储文件描述符信息的红黑树中查找指定的fd对应的epitem实例
  dpi = ep_find(ep, fd);
​
  error = -EINVAL;
  switch (op) {
  case EP_CTL_ADD:
    // 若要添加的fd不存在，则调用ep_insert()插入红黑树
    if (!dpi)
      error = ep_insert(ep, &pfd);
    else
      // 若已存在，则返回EEXIST错误
      error = -EEXIST;
    break;
  case EP_CTL_DEL:
    if (dpi)
      error = ep_remove(ep, dpi);
    else
      error = -ENOENT;
    break;
  case EP_CTL_MOD:
    if (dpi) {
      dpi->pfd.events = events;
      error = 0;
    } else
      error = -ENOENT;
    break;
  }
​
  up_write(&ep->acsem);
​
eexit_2:
  fput(file);
eexit_1:
  return error;
}


/*
 * 实现eventpoll file的event wait接口
 * kernel part of the user space epoll_wait(2)
 *
 * @eptd 文件描述符
 * @events 
 * @timeout 
 */
asmlinkage int sys_epoll_wait(int epfd, struct pollfd const **events, int timeout)
{
  int error = -EBADF;
  void *eaddr;
  struct file *file;
  struct eventpoll *ep;
  struct evpoll dvp;
​
  file = fget(epfd);
  if (!file)
    goto eexit_1;
​
  /*
   * We have to check that the file structure underneath the file descriptor
   * the user passed to us _is_ an eventpoll file.
   */
  error = -EINVAL;
  if (!IS_FILE_EPOLL(file))
    goto eexit_2;
​
  ep = file->private_data;
​
  /*
   * It is possible that the user crseated an eventpoll file by open()ing
   * the corresponding /dev/ file and he did not perform the correct
   * initialization required by the old /dev/epoll interface. This test
   * protect us from this scenario.
   */
  error = -EINVAL;
  if (!atomic_read(&ep->mmapped))
    goto eexit_2;
​
  dvp.ep_timeout = timeout;
  error = ep_poll(ep, &dvp);
  if (error > 0) {
    eaddr = (void *) (ep->vmabase + dvp.ep_resoff);
    if (copy_to_user(events, &eaddr, sizeof(struct pollfd *)))
      error = -EFAULT;
  }
​
eexit_2:
  fput(file);
eexit_1:
  return error;
}


 event.events = EPOLLIN | EPOLLET;//ET 边沿触发模式
 event.events = EPOLLIN;          //默认 LT触发模式 
// 水平触发
#include<ctype.h>
#include<unistd.h>
#include<stdio.h>
#include<strings.h>
#include<sys/wait.h>
#include<sys/epoll.h>
#include<time.h>
#define MAXSIZELINE 10

int main(void)
{
	int pfd[2];//创建管道准备
	int efd,i;
	pid_t pid;
	char buf[MAXSIZELINE];
	char ch = '1';
	pipe(pfd);//创建管道
	
	pid = fork();
	if(pid == 0)//子进程 写
	{
		close(pfd[0]);//关闭 读
		while(1)
		{
			for(i = 0; i < MAXSIZELINE / 2;i++)
				buf[i] = ch;
			ch++;
			for(i; i < MAXSIZELINE;i++)
				buf[i] = ch;
			ch++;
			
			write(pfd[1],buf,MAXSIZELINE);//
			sleep(3);//每次休眠3秒再次写入
			printf("write again!\n");
		}
			close(pfd[1]);
	}
	else if(pid > 0)//父进程 读
	{
       	close(pfd[1]);
		struct epoll_event event;
		struct epoll_event resevent[MAXSIZELINE];
		int res , len;
		efd = epoll_create(MAXSIZELINE);
		//event.events = EPOLLIN | EPOLLET;//ET 边沿触发模式
		event.events = EPOLLIN;          //默认 LT触发模式 
		event.data.fd = pfd[0];
		epoll_ctl(efd,EPOLL_CTL_ADD,pfd[0],&event);
		printf("epoll_wait begin \n");
		while(1)
		{
		    printf("epoll_wait again \n");
			res = epoll_wait(efd,resevent,MAXSIZELINE,-1);
			printf("res = %d",res);
                        printf("\n");
			if(resevent[0].data.fd == pfd[0])
			{
				len = read(pfd[0],buf,MAXSIZELINE/2);//测试触发模式，只读一半的数据，留一半在缓冲区内
				write(STDOUT_FILENO,buf,len);
			}
		}
		
	}
}


