#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <asm/ptrace.h>
#include <asm/user.h>
#include <asm/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <jni.h>
#include <elf.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <utils/Log.h>
#include <android/log.h>
#include <inject.h>
#include <inject.c>
#include <properties.h>
//#define LOG_TAG "debug"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define PROP_OPEN_CALL_COUNT    "persist.sys.open.callcount"
#define PROP_OLD_OPEN_ADDR      "persist.sys.open.old"
#define PROP_NEW_OPEN_ADDR      "persist.sys.open.new"

//extern int ptrace_readdata( pid_t pid,  uint8_t *src, uint8_t *buf, size_t size );
//extern int ptrace_writedata( pid_t pid, uint8_t *dest, uint8_t *data, size_t size );
/*--
 void* get_module_base(pid_t pid, const char* module_name) {
 FILE *fp;
 long addr = 0;
 char *pch;
 char filename[32];
 char line[1024];

 if (pid < 0) {
 // self process
 snprintf(filename, sizeof(filename), "/proc/self/maps", pid);
 } else {
 snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
 }

 fp = fopen(filename, "r");

 if (fp != NULL) {
 while (fgets(line, sizeof(line), fp)) {
 if (strstr(line, module_name)) {
 pch = strtok(line, "-");
 addr = strtoul(pch, NULL, 16);

 if (addr == 0x8000)
 addr = 0;

 break;
 }
 }

 fclose(fp);
 }

 return (void *) addr;
 }
 */
int hook_entry(int * a) {
	//unsigned long *old_open_addr;
	//unsigned long *new_open_addr;
	unsigned long old_open_addr;
    unsigned long new_open_addr;
	unsigned long old_close_addr;
    unsigned long new_close_addr;

	LOGD("hello ARM! %s pid:%d\n", a, getpid());
	void *handle;
	int (*fcn)(unsigned long *param,unsigned long *param1,unsigned long *param2,unsigned long *param3);
	int target_pid = getpid();

	handle = dlopen("/dev/libhook_open.so", RTLD_NOW);
	LOGD("The Handle of libhook_open: %x\n",handle);

	if (handle == NULL) {
		LOGD("Failed to load libhook_open.so: %s\n", dlerror());
		//fprintf(stderr, "Failed to load libsthc.so: %s\n", dlerror());
		return 1;
	}
	// void * binder_addr = get_module_base(getpid(), "/dev/libhook_open.so");
	//  LOGD("open path %x\n",binder_addr);
	LOGD("find it pre %x\n", fcn);
	fcn = dlsym(handle, "do_hook");
	if (fcn != NULL)
		LOGD("find it %x\n", fcn);
	fcn(&old_open_addr, &new_open_addr,&old_close_addr,&new_close_addr);
	//ȡold_open_addr��ַ
	LOGD("[+] Get old address global  %x\n", old_open_addr);
/*	char value[PROPERTY_VALUE_MAX] = { '\0' };
	do {
		LOGD("get old address\n");
		sleep(1);
		property_get(PROP_OLD_OPEN_ADDR, value, "0");
	} while (strcmp(value, "0") == 0);
	old_open_addr = atoi(value);
	LOGD("[+] Get old address property  %x\n", old_open_addr);*/
	//ȡnew_open_addr��ַ
	LOGD("[+] Get new address global  %x\n", new_open_addr);
	LOGD("[+] Get old address global  %x\n", old_close_addr);
	LOGD("[+] Get new address global  %x\n", new_close_addr);
/*	do {
		LOGD("get new address\n");
		sleep(1);
		property_get(PROP_NEW_OPEN_ADDR, value, "0");
	} while (strcmp(value, "0") == 0);
	new_open_addr = atoi(value);
	LOGD("[+] Get new address property  %x\n", new_open_addr);*/

	//��̬��libbinder.so��
	/*void * binder_addr = get_module_base(target_pid,
			"/system/lib/libbinder.so");
	LOGD("[+] binder path %x\n", binder_addr);
	if (binder_addr == NULL) {
		LOGD("Failed to get module base!\n");
		return 1;
	}*/

	//���ļ���ʽ��libbinder.so
	/*int fp = 0;
	if ((fp = open("/system/lib/libbinder.so", O_RDONLY)) == 0) {
		LOGD("can not open file\n");
		exit(0);
	}*/
	//��̬��libc.so��
	void * binder_addr = get_module_base(target_pid,
				"/system/lib/libnativehelper.so");
		LOGD("[+] binder path %x\n", binder_addr);
		if (binder_addr == NULL) {
			LOGD("Failed to get module base!\n");
			return 1;
		}

	//���ļ���ʽ��libc.so
	int fp = 0;
		if ((fp = open("/system/lib/libnativehelper.so", O_RDONLY)) == 0) {
				LOGD("can not open file\n");
				exit(0);
		}

	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) malloc(sizeof(Elf32_Ehdr));
	read(fp, ehdr, sizeof(Elf32_Ehdr));
	if (ehdr == NULL) {
		LOGD("Failed to read ehdr!\n");
		return 1;
	}
	//����Elfͷ��ʽ�������
	unsigned long shdr_addr = ehdr->e_shoff;
	LOGD("[+] Section header table file offset %x\n", ehdr->e_shoff);
	int shnum = ehdr->e_shnum;
	LOGD("[+] Section header table entry count %x\n", shnum);
	int shent_size = ehdr->e_shentsize;
	LOGD("[+] Section header table entry size %x\n", shent_size);
	unsigned long stridx = ehdr->e_shstrndx;
	LOGD("[+] Section header string table index %x\n", stridx);
	int type = ehdr->e_type;
	LOGD("[+] Object file type %x\n", type);
	int version = ehdr->e_version;
	LOGD("[+] Object file version %x\n", version);
	int fd = fp;
	// ��ȡSection Header�й����ַ�������������õ���ߴ��λ��
	lseek(fd, shdr_addr + stridx * shent_size, SEEK_SET);
	Elf32_Shdr *shdr = (Elf32_Shdr *) malloc(sizeof(Elf32_Shdr));
	read(fd, shdr, shent_size);
	// ���ݳߴ�����ڴ�
	char * string_table = (char *) malloc(shdr->sh_size);
	lseek(fd, shdr->sh_offset, SEEK_SET);

	// ���ַ��������ݶ���
	read(fd, string_table, shdr->sh_size);
	//�����±���Section Header������Ϊ.got���Section��
	int *out_addr;
	unsigned int out_size;
	LOGD("[+] Out_addr in GOT table before %x\n", out_addr);
	LOGD("[+] Out_size of func in GOT table before %x\n", out_size);
	lseek(fd, shdr_addr, SEEK_SET);
	int i;
	for (i = 0; i < shnum; i++) {
		read(fd, shdr, shent_size);

		if (shdr->sh_type == SHT_PROGBITS) {
			int name_idx = shdr->sh_name;
			if (strcmp(&(string_table[name_idx]), ".got") == 0) {
				/* ���� GOT �� */

				out_addr = binder_addr + shdr->sh_offset;
				out_size = shdr->sh_size;
				LOGD("[+] Out_addr in GOT table %x\n", out_addr);
				LOGD("[+] Out_size of func in GOT table %x\n", out_size);
				break;
			}
		}
	}
	LOGD("[+] Out_addr in GOT table %x\n", out_addr);
	LOGD("[+] Out_size of func in GOT table %x\n", out_size);
//	int b = *out_addr;
//	LOGD("[+] Out_addr in GOT %x\n", b);

	LOGD("[+] old open address 20000 %x\n", old_open_addr);

	/*long word = ptrace(PTRACE_PEEKTEXT, target_pid, out_addr, NULL);
	LOGD("[+] word %x\n", word);
	long elfword = ptrace(PTRACE_PEEKTEXT, target_pid, binder_addr, NULL);
	LOGD("[+] wordelf %x\n", elfword);*/


	int v = *(int *) binder_addr;
	LOGD("[+] Out_addr in GOT %x\n", v);
	//������Hook�ͺð��ˣ�
	uint8_t got_item = 0;
	LOGD("[+] target pid %x\n", target_pid);

	LOGD("[+] old open address 22223 %x\n", old_open_addr);
    unsigned long old_add=old_open_addr;
    unsigned long new_add=new_open_addr;
	ptrace_readdata(target_pid, out_addr, &got_item, 4);
	LOGD("[+] Find got_item %x \n", got_item);
	LOGD("[+] old open address 3333 %x\n", old_add);
	LOGD("[+] old open address 3333 %x\n", old_open_addr);//Ϊʲôֵ�ı����أ�������
	LOGD("[+] new open address 3333 %x\n", new_add);
    LOGD("[+] new open address 3333 %x\n", new_open_addr);//Ϊʲôֵ�ı����أ�������


	for (i = 0; i < out_size; i++) {
		int got_item = *out_addr;
		LOGD("[+] Find got item %x \n", got_item);
		if (got_item == old_add) {
			/* !!! �õ��� open ��ַ !!! �ĳ����ǵġ� */
			LOGD("[+]---------- Find open and replace \n-----------");
			//ptrace_writedata(target_pid, out_addr, &new_open_addr,
			//	sizeof(new_open_addr));
			(*out_addr) = new_open_addr;
			//
		}
		else if (got_item == old_close_addr)
		{
			LOGD("[+]---------- Find close and replace \n-----------");
			(*out_addr) = new_close_addr;
		}
		else if (got_item == new_open_addr||got_item == new_open_addr) {
			/* �Ѿ������ǵ��ˣ����ظ�Hook�� */
			break;
		}
		out_addr++;
	}
/*
	//�����±���Section Header������Ϊ.rel.plt���Section��
		int *rel_addr;
		unsigned int rel_size;
		LOGD("[+] Out_addr in REL.PLT table before %x\n", rel_addr);
		LOGD("[+] Out_size of func in REL.PLT table before %x\n", rel_size);
		lseek(fd, shdr_addr, SEEK_SET);
		for (i = 0; i < shnum; i++) {
			read(fd, shdr, shent_size);

			if (shdr->sh_type == SHT_REL | ) {
				int name_idx = shdr->sh_name;
				if (strcmp(&(string_table[name_idx]), ".rel.plt") == 0) {
					 ���� REL.PLT ��

					rel_addr = binder_addr + shdr->sh_offset;
					rel_size = shdr->sh_size;
					LOGD("[+] Out_addr in REL.PLT table %x\n", rel_addr);
					LOGD("[+] Out_size of func in REL.PLT table %x\n", rel_size);
					break;
				}
			}
		}
		int *rel_addr1=rel_addr;
		LOGD("[+] Out_addr in REL.PLT table %x\n", rel_addr);
		LOGD("[+] Out_size of func in REL.PLT table %x\n", rel_size);
		got_item = 0;
		LOGD("[+] target pid %x\n", target_pid);
		ptrace_readdata(target_pid, rel_addr, &got_item, 4);
		LOGD("[+] Find got_item for REL.PLT %x \n", got_item);

		for (i = 0; i < rel_size; i++) {
			int got_item = *rel_addr;
			//LOGD("[+] Rel_addr in REL.PLT %x\n", b);
			LOGD("[+] Find got item for Rel %x \n", got_item);
			if (got_item == old_open_addr) {
				 !!! �õ��� open ��ַ !!! �ĳ����ǵġ�
				LOGD("[+] Find open and replace \n");
				//ptrace_writedata(target_pid, out_addr, &new_open_addr,
				//	sizeof(new_open_addr));
				(*out_addr) = new_open_addr;
				//
				break;
			} else if (got_item == new_open_addr) {
				 �Ѿ������ǵ��ˣ����ظ�Hook��
				break;
			}
			rel_addr++;
		}
*/

	return 0;
}
