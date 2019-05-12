#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>


pthread_t th[100]; //inisialisasi array untuk menampung thread dalam kasus ini ada 100 thread
typedef struct cron{
    int minute;
    int hour;
    int day_of_month;
    int month;
    int day_of_week;
    char path_eksekutor[256];
    char path_target[256];
}cron_conf;

cron_conf conf[100];

int reset = 0;        
int thread_num = 0;

time_t t1;
struct tm *t;

int run(cron_conf conf)
{
    return  (t->tm_sec==0) &&                                              // Cron cuma jalan kalau detiknya nol
            (conf.minute==99 || conf.minute==t->tm_min) &&                 // Cek menit
            (conf.day_of_month==99 || conf.day_of_month==t->tm_mday) &&    // Cek hari dalam bulan
            (conf.day_of_week==99 || conf.day_of_week==t->tm_wday) &&      // Cek hari dalam mingguan
            (conf.hour==99 || conf.hour==t->tm_hour) &&                    // Cek jam
            (conf.month==99 || conf.month==t->tm_mon);                     // Cek bulan
}

void seperate(char path[1000], cron_conf *conf1)
{
    memset(conf1->path_target, '\0', sizeof(conf1->path_target));
    memset(conf1->path_eksekutor, '\0', sizeof(conf1->path_eksekutor));
    int i, a, number=0;
    for(i=0, a=0; i<strlen(path) && a<5; i++)
    {
        if(path[i]==' ')
        {
            if(a==0) conf1->minute=number;
            else if(a==1) conf1->hour=number;
            else if(a==2) conf1->day_of_month=number;
            else if(a==3) conf1->month=number;
            else if(a==4) conf1->day_of_week=number;
            a++;
            number=0;
        }
        else
        {
            if(path[i]=='*') number=99;
            else
            {
                number*=10;
                number+=(path[i]-'0');
            }
        }
    } 

    int j=0;

    for(i;i<strlen(path); i++)
    {
        if(path[i]==' ')
        {
            a++;
            j=0;
        }
        else
        {
            if(a==5) conf1->path_eksekutor[j]=path[i];
            else conf1->path_target[j]=path[i];
            j++;
        }
    }    
}

void* execute_time(void* i){
    // Ekstrak parameter
    int number = *((int *) i);
    free(i);
    
    // Disini bagian konversi dari *** jadi detik, masih belum dibuat
    while (!reset)
    {
        if(run(conf[number]))
        {
            pid_t running;
            running =fork();
            int status;
            if(running == 0)
            {
                // Jalanin pake exec
                // Karena path_eksekutor adalah path seperti /bin/bash maka jadikan argumen pertama
                // Karena path_targer merupakan path yang dieksekusi seperti /home/[user]/Document/haha.sh maka jadikan argumen kedua
                // printf("thread is executed %s\n",conf[number].path_eksekutor);
                execv(conf[number].path_eksekutor, conf[number].path_target);
            }
            else
            {
                // Tunggu eksekusi berhasil dulu
                while((wait(&status))>0);
            }
        }
        sleep(1);
    }
    // printf("thread is killed %s\n",conf[number].path_eksekutor);
    return NULL;
}


int main() {
  FILE* config;
  struct stat sb;
  time_t last_modify;
  char types[10];
  char path[1000];
  pid_t pid, sid;

  pid = fork();

  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  umask(0);

  sid = setsid();

  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

//   if ((chdir("/")) < 0) {
//     exit(EXIT_FAILURE);
//   }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  while(1) {
        stat("config.txt",&sb);
        time(&t1);
        t = localtime(&t1);
        if (last_modify != sb.st_mtime)
        {
            // system("clear");
            config = fopen("config.txt","r");
            reset = 1;
            for (int i = 0; i < thread_num; i++)
            {
                pthread_join(th[i],NULL);
            }
            reset = 0;
            thread_num=0;
            while (fgets(path, 1000, config))
            {
                // Digunakan untuk passing argumen threadnya
                int *arg = malloc(sizeof(*arg));
                *arg = thread_num;

                // Pemisahan * * * * * /bin/bash /home/[user]/Documents/haha.sh
                seperate(path, &conf[thread_num]);
                
                // Membuat thread dengan argument thread_num sebagai index access ke cron_conf
                // printf("Thread started for %s\n",conf[thread_num].path_eksekutor);
                pthread_create(&(th[thread_num]),NULL,execute_time,arg);
                
                // Increment untuk jumlah threadnya
                thread_num++;
            }
            
            // fflush(config);
            last_modify=sb.st_mtime;
            fclose(config);
        }
        reset = 0;
  }
  
  exit(EXIT_SUCCESS);
}
