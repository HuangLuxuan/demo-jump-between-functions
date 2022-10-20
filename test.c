#include <stdio.h>
#include <stdlib.h>

void dummy(){}

#ifndef __x86_64__
int *getEBP(){
    int *notused;
    return (int*)(void*)*(&notused+1);
}

/*
caller_ip_ret
caller_ebp_backup   <-ebp_backup
====
caller stack
====
int fix_esp                         ip_ret
void* ip                            ebp_backup      <-dummy_ebp
ip_ret
ebp_backup          <-ebp
*/
void jump(void* ip,int fix_esp,int fpd, ...){
    int *ebp=getEBP();
    int *caller_ebp_backup=(int*)(void*)(*ebp+fpd);
    int *target_ebp=(int*)((int)caller_ebp_backup-fix_esp)-2;
    void *ip_target=ip;
    int *ebp_target=caller_ebp_backup;
    printf("ebp=%p\n*ebp=%p\n",ebp,(void*)*ebp);
    if(fix_esp>=0){
        if(target_ebp<ebp){
            printf("error: no enough space in stack: caller ebp=%p, target esp is %d bytes below caller ebp, current ebp=%p, free space of at least 2 stack items long is expected.\nstack overflow!!\n",caller_ebp_backup,fix_esp,ebp);
            exit(-1);
        }
        if(((int)target_ebp-(int)ebp)%8!=0){
            printf("error: stack not aligned: target ebp=%p, current ebp=%p\n, (target-current)%%8 should be 0\n",target_ebp,ebp);
            exit(-1);
        }
        while(target_ebp!=ebp){
            *(target_ebp+1)=(int)ip_target;
            *target_ebp=(int)ebp_target;
            ip_target=(void*)(3+(int)dummy);
            ebp_target=target_ebp;
            target_ebp-=2;
        }
        *(ebp+1)=(int)ip_target;
        *ebp=(int)ebp_target;
        return;
    }
    *(ebp+1)=(int)ip;
}
#else
long long* getRBP(){
    long long *notused;
    return (long long*)(void*)*(&notused+1);
}

/*
caller_ip_ret
caller_rbp_backup   <-rbp_backup
====
caller stack
====
int fix_rsp                         ip_ret
void* ip                            rbp_backup      <-dummy_rbp
ip_ret
rbp_backup          <-rbp
*/
void jump(void* ip,int fix_rsp,int fpd, ...){
    long long *rbp=getRBP();
    long long *caller_rbp_backup=(long long*)(void*)(*rbp+fpd);
    long long *target_rbp=(long long*)((long long)caller_rbp_backup-fix_rsp)-2;
    void *ip_target=ip;
    long long *rbp_target=caller_rbp_backup;
    printf("rbp=%p\n*rbp=%p\n",rbp,(void*)*rbp);
    if(fix_rsp>=0){
        if(target_rbp<rbp){
            printf("error: no enough space in stack: caller rbp=%p, target rsp is %d bytes below caller rbp, current rbp=%p, free space of at least 2 stack items long is expected.\nstack overflow!!\n",caller_rbp_backup,fix_rsp,rbp);
            exit(-1);
        }
        if(((long long)target_rbp-(long long)rbp)%16!=0){
            printf("error: stack not aligned: target rbp=%p, current rbp=%p\n, (target-current)%%16 should be 0\n",target_rbp,rbp);
            exit(-1);
        }
        while(target_rbp!=rbp){
            *(target_rbp+1)=(long long)ip_target;
            *target_rbp=(long long)rbp_target;
            ip_target=(void*)(4+(long long)dummy);
            rbp_target=target_rbp;
            target_rbp-=2;
        }
        *(rbp+1)=(long long)ip_target;
        *rbp=(long long)rbp_target;
        return;
    }
    *(rbp+1)=(long long)ip;
}
#endif
int getFPD(void *fun){
    int fpd=0;
    unsigned char *p=(char*)fun;
    if(sizeof(void*)==4){
        if(*p++!=0x55)return 0;//push ebp
        if(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC)return 0;//mov ebp,esp
        while(*p>=0x50&&*p<=0x57){
            p++;
            fpd+=4;
        }
        if(*p==0x81&&*(p+1)==0xEC){
            fpd+=*(int*)(void*)(p+2);//sub esp,0x????????
            p+=6;
        }
        else if(*p==0x81&&*(p+1)==0xE8){
            fpd+=-*(int*)(void*)(p+2);//add esp,0x????????
            p+=6;
        }
        else if(*p==0x83&&*(p+1)==0xEC){
            fpd+=*(p+2);//sub esp,+/-0x??
            p+=3;
        }
        else if(*p==0x83&&*(p+1)==0xE8){
            fpd+=-*(p+2);//add esp,+/-0x??
            p+=3;
        }
        if(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC)return fpd;//mov ebp,esp
        if(*p==0x8D&&(*(p+1)&0x3F)==0x2C){
            if((*(p+1)&0xC0)==0xC0)return 0;//err
            if(*(p+2)!=0x24)return 0;//err
            if((*(p+1)&0xC0)==0x00)return fpd;//lea ebp,[esp]
            if((*(p+1)&0xC0)==0x40)return fpd-*(p+3);//lea ebp,[esp+/-0x??]
            if((*(p+1)&0xC0)==0x80)return fpd-*(int*)(void*)(p+3);//lea ebp,[esp+0x????????]
        }
    }
    else if(sizeof(void*)==8){
        if(*p++!=0x55)return 0;//push rbp
        if(*p==0x48&&(*(p+1)==0x89&&*(p+2)==0xE5||*(p+1)==0x8B&&*(p+2)==0xEC))return 0;//mov rbp,rsp
        while(*p>=0x50&&*p<=0x57){
            p++;
            fpd+=8;
        }
        if(*p++!=0x48)return 0;
        if(*p==0x81&&*(p+1)==0xEC){
            fpd+=*(long long*)(void*)(p+2);//sub rsp,0x????????????????
            p+=6;
        }
        else if(*p==0x81&&*(p+1)==0xE8){
            fpd+=-*(long long*)(void*)(p+2);//add rsp,0x????????????????
            p+=6;
        }
        else if(*p==0x83&&*(p+1)==0xEC){
            fpd+=*(p+2);//sub rsp,+/-0x??
            p+=3;
        }
        else if(*p==0x83&&*(p+1)==0xE8){
            fpd+=-*(p+2);//add rsp,+/-0x??
            p+=3;
        }
        printf("%02X %02X %02X %02X\n",*p,*(p+1),*(p+2),*(p+3));
        if(*p++!=0x48)return 0;
        if(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC)return fpd;//mov rbp,rsp
        if(*p==0x8D&&(*(p+1)&0x3F)==0x2C){
            if((*(p+1)&0xC0)==0xC0)return 0;//err
            if(*(p+2)!=0x24)return 0;//err
            if((*(p+1)&0xC0)==0x00)return fpd;//lea rbp,[rsp]
            if((*(p+1)&0xC0)==0x40)return fpd-*(p+3);//lea rbp,[rsp+/-0x??]
            if((*(p+1)&0xC0)==0x80)return fpd-*(int*)(void*)(p+3);//lea rbp,[rsp+0x????????]
        }
    }
    return 0;
}

int stack(void *fun){
    int additional=0;
    unsigned char *p=(char*)fun;
    if(sizeof(void*)==4){
        if(*p++!=0x55)return 0;//push ebp
        if(!(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC))return 0;//mov ebp,esp
        p+=2;
        while(*p>=0x50&&*p<=0x57){
            p++;
            additional+=4;
        }
        if(*p==0x81&&*(p+1)==0xEC)return *(int*)(void*)(p+2)+additional;//sub esp,0x????????
        if(*p==0x81&&*(p+1)==0xE8)return -*(int*)(void*)(p+2)+additional;//add esp,0x????????
        if(*p==0x83&&*(p+1)==0xEC)return *(p+2)+additional;//sub esp,+/-0x??
        if(*p==0x83&&*(p+1)==0xE8)return -*(p+2)+additional;//add esp,+/-0x??
    }
    else if(sizeof(void*)==8){
        if(*p++!=0x55)return 0;//push rbp
        if(*p++!=0x48)return 0;
        if(!(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC))return 0;//mov rbp,rsp
        p+=2;
        while(*p>=0x50&&*p<=0x57){
            p++;
            additional+=8;
        }
        if(*p++!=0x48)return 0;
        if(*p==0x81&&*(p+1)==0xEC)return (int)*(long long*)(void*)(p+2)+additional;//sub rsp,0x????????????????
        if(*p==0x81&&*(p+1)==0xE8)return (int)-*(long long*)(void*)(p+2)+additional;//add rsp,0x????????????????
        if(*p==0x83&&*(p+1)==0xEC)return *(p+2)+additional;//sub rsp,+/-0x??
        if(*p==0x83&&*(p+1)==0xE8)return -*(p+2)+additional;//add rsp,+/-0x??
    }
    return 0;
}

void* skip(void * fun){
    unsigned char *p=(char*)fun;
    if(sizeof(void*)==4){
        //printf("%02X\n",*p);
        if(*p++!=0x55)return NULL;//push ebp
        //printf("%02X %02X\n",*p,*(p+1));
        if(!(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC))return NULL;//mov ebp,esp
        p+=2;
        while(*p>=0x50&&*p<=0x57)p++;
        //printf("%02X %02X\n",*p,*(p+1));
        if(*p==0x81&&*(p+1)==0xEC)return (void*)(p+6);//sub esp,0x????????
        if(*p==0x81&&*(p+1)==0xE8)return (void*)(p+6);//add esp,0x????????
        if(*p==0x83&&*(p+1)==0xEC)return (void*)(p+3);//sub esp,+/-0x??
        if(*p==0x83&&*(p+1)==0xE8)return (void*)(p+3);//add esp,+/-0x??
        return (void*)p;
    }
    else if(sizeof(void*)==8){
        //printf("%02X\n",*p);
        if(*p++!=0x55)return NULL;//push rbp
        //printf("%02X %02X %02X\n",*p,*(p+1),*(p+2));
        if(*p++!=0x48)return NULL;
        if(!(*p==0x89&&*(p+1)==0xE5||*p==0x8B&&*(p+1)==0xEC))return NULL;//mov rbp,rsp
        p+=2;
        while(*p>=0x50&&*p<=0x57)p++;
        //printf("%02X %02X %02X\n",*p,*(p+1),*(p+2));
        if(*p!=0x48)return (void*)p;
        p++;
        if(*p==0x81&&*(p+1)==0xEC)return (void*)(p+10);//sub rsp,0x????????????????
        if(*p==0x81&&*(p+1)==0xE8)return (void*)(p+10);//add rsp,0x????????????????
        if(*p==0x83&&*(p+1)==0xEC)return (void*)(p+3);//sub rsp,+/-0x??
        if(*p==0x83&&*(p+1)==0xE8)return (void*)(p+3);//add rsp,+/-0x??
        return (void*)(p-1);
    }
    return NULL;
}

int fun2(){
    int var_in_fun2;
    printf("fun2()\n&var_in_fun2=%p\nvar_in_fun2=%d\n",&var_in_fun2,var_in_fun2);
}

int fun1(){
    int var_in_fun1=114514;
    printf("fun1()\n&var_in_fun1=%p\nvar_in_fun1=%d\n",&var_in_fun1,var_in_fun1);
    //jump(3+(int)(void*)fun2);//+3跳过push ebp和mov ebp,esp
    printf("jump to fun2()+0x%x, fun2() stack allocated %d bytes.\n",(int)((char*)skip((void*)fun2)-(char*)fun2),stack((void*)fun2));
    jump(skip((void*)fun2),stack((void*)fun2),getFPD((void*)fun1),0LL,0LL);//跳过push ebp和move ebp,esp以及sub esp,xxx
    printf("fun1()??\n");
}

int main(){
    int i;
    printf("&i=%p\n",&i);
    fun1();
    printf("&i=%p\n",&i);
    return 0;
}