# FP_SISOP19_E15
Untuk FP kami mendapatkan soal daemon.  
Soalnya meminta untuk membuat crontab, tetapi untuk mengganti konfigurasi tidak perlu merestart daemonnya, cuma perlu merubah file config saja.  

dalam FP kami terdapat beberapa fungsi :  

Fungsi ini berguna untuk melakukan pengecekkan terhadap kapan settingan cron harus berjalan.

```
int run(cron_conf conf)
{
    return  (t->tm_sec==0) &&                                              // Cron cuma jalan kalau detiknya nol
            (conf.minute==99 || conf.minute==t->tm_min) &&                 // Cek menit
            (conf.day_of_month==99 || conf.day_of_month==t->tm_mday) &&    // Cek hari dalam bulan
            (conf.day_of_week==99 || conf.day_of_week==t->tm_wday) &&      // Cek hari dalam mingguan
            (conf.hour==99 || conf.hour==t->tm_hour) &&                    // Cek jam
            (conf.month==99 || conf.month==t->tm_mon);                     // Cek bulan
}
```


Fungsi ini berguna untuk memisahkan konfigurasi dari cron nya.
Format cron yang digunakan adalah :    
  
\* \* \* \* \* /path/ke/eksekusi /path/ke/target

Jadi difungsi ini akan dipisahkan dari confignya lalu dimasukkan ke struct. Pada fungsi yang berguna untuk memisahkan setiap konfigurasi dari cron, kami menggunakan dua kali perulangan. Perulangan pertama digunakan untuk menyimpan angka angka dalam configan cron sedangkan perulangan kedua digunakan untuk menyalin kedua string yang ada pada settingan cron.
```
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
```

Ini adalah fungsi thread yang akan berajalan. Pada fungsi ini dilakukan eksekusi dan pemanggilan pengecekkan jam. Selama flag reset tidak terpanggil, maka thread akan selalu berjalan sesuai dengan konfigurasinya.

Jika reset diset, maka pada semua thread akan  berhenti.
Jika sudah waktunya untuk menjalankan exec, maka akan dibuat fork untuk mengeksekusi exec. Thread akan menunggu untuk selesai dieksekusi, lalu thread akan menunggu lagi.

```
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
```

berikut adalah fungsi utamananya :  
Pertama akan selalu dicek kondisi dari file konfigurasi. Jika file konfigurasi memiliki perubahan/modify maka semua thread akan dihentikan. Program menunggu semua thread untuk berhenti, lalu proses inisialisasi thread dimulai.  
Pertama-tama dibaca dari file config, lalu dipisahkan.
Setelah itu thread dimulai.

```
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
  ```

  Pada program kami juga menggunakan struct :  
  ```
  typedef struct cron{
    int minute;
    int hour;
    int day_of_month;
    int month;
    int day_of_week;
    char path_eksekutor[256];
    char path_target[256];
}cron_conf;
```
Struct ini menyimpan data konfigurasi dari file config.
