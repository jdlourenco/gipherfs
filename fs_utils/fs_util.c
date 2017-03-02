#include "fs_util.h"

/**
 *
 * Initializes cache: creates backing and mount dir if they don't already exist
 * 
 *
 */
//int init_cache(cache* file_cache, char* config_path) {

double get_current_timestamp() {
  struct timespec tp;
  double timestamp;

  clock_gettime(CLOCK_REALTIME, &tp);
  timestamp = tp.tv_sec + (tp.tv_nsec / BILLION);

  return timestamp;
}


char* get_current_time() {
  time_t rawtime;
  struct tm *timeinfo;
  char *date_str;

  date_str = (char*) malloc(sizeof(char)*DATE_STR_LEN + 1);

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  sprintf(date_str, "%d%02d%02d%02d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  return date_str;
}

off_t get_file_size(int fd) {
  int fd_dup = dup(fd);
  off_t file_size = lseek(fd_dup, 0, SEEK_END);

  close(fd_dup);

  return file_size;
}

int get_block_size(int fd) {
  int block_size;

  if(ioctl(fd, FIGETBSZ, &block_size) < 0) {
    printf("unable to get block size\n");
    return -1;
  }
  else {
    return block_size;
  }
}

char* get_instrumentation_path(char *type, char* pwd) {
  char *path;
  char *date;

  date = get_current_time();

  int str_size = strlen(pwd) + strlen(INSTR_DIR) + strlen(type) + strlen(date) + 1;

  path = (char*) malloc(sizeof(char*)*str_size);
  memset(path, 0, sizeof(char*)*str_size);

  strncpy(path, pwd, strlen(pwd));
  strncat(path, INSTR_DIR, strlen(INSTR_DIR));
  strncat(path, type, strlen(type));
  strncat(path, date, strlen(date));

  free(date);

  return path;
}

char* get_back_path(cache* file_cache, const char* path) {
  char* back_path=NULL;

  back_path  = (char*) malloc(sizeof(char) * (strlen(file_cache->backing_dir)+strlen(path)+1));
  strcpy(back_path,file_cache->backing_dir);
  strcat(back_path,path);

  return back_path;
}

char* remove_prefix_str(char* str, char* prefix) {
  int prefix_len = strlen(prefix);
  char* result = (char*) malloc(sizeof(char)*(strlen(str)-prefix_len+1));
  strcpy(result,str+prefix_len);
  return result;
}

int init_cache(cache* file_cache, char* back_dir, char* mount_dir) {
  int fd;

  file_cache->backing_dir = back_dir;
  file_cache->mount_dir   = mount_dir;

  /**
   * create mount_dir if it doesn't exist
   */
  fd = open_create_dir(file_cache->mount_dir);
  if(fd < 0) {
    printf("unable to open/create %s\n", file_cache->mount_dir);
    return -1;
  }
  close(fd);

 /**
   * create backing_dir if doesn't exists
   * get backing_dir fs block size to use as cache block size 
   */
  fd = open_create_dir(file_cache->backing_dir);
  if(fd < 0) {
    printf("unable to open/create %s\n", file_cache->backing_dir);
    return -1;
  }

  file_cache->block_size = get_block_size(fd);

  close(fd);

  return 0;
}

int open_create_dir(char* dir_path) {
  int fd;
  struct stat sb;

  if(!(stat(dir_path, &sb) == 0 && S_ISDIR(sb.st_mode))) {
    printf("creating %s\n", dir_path);
    if(mkdir(dir_path, S_IRWXU)) {
      perror("");
      return -errno;
    }
  }

  fd = open(dir_path, O_DIRECTORY);
  if(fd == -1) {
    perror("");
    return -errno;
  }
  return fd;
}

/*
char* physical_block_to_string(unsigned long long int phy_block) {
  char* res = (char*) malloc(sizeof(char) * MAX_BLOCK_STR_SIZE);

  sprintf(res, "%llu", phy_block);
  res = realloc(res, strlen(res)+1);

  return res;
}
*/


// backing_fd already points to the desired position of the file
/*size_t read_cache_block(int backing_fd, char* buffer, size_t size, off_t offset) {
  
  //int block_fd;
  //unsigned long long int phy_block;
  size_t res;//, res2;
  //char *phy_block_str, *block_cache_path;

  //phy_block_str = NULL;
  //phy_block = get_physical_block(backing_fd, file_cache->block_size);
  //printf("physical_block=%llu\n", phy_block);

  //phy_block_str = physical_block_to_string(phy_block);

  //printf("physical_block_str=%s\n", phy_block_str);
  // +2 because there needs to be room for the extra /
  //block_cache_path  = (char*) malloc(sizeof(char) * (strlen(file_cache->cache_dir) + strlen(phy_block_str) + 2));
  //strcpy(block_cache_path, file_cache->cache_dir);
  //strcat(block_cache_path,PATH_DELIM);
  //strcat(block_cache_path,phy_block_str);

  //printf("block_path=%s\n", block_cache_path);

  //if(access(block_cache_path, F_OK) == -1) {
  //  printf("lets copy the file to cache\n");
  res = pread(backing_fd, buffer, size, offset);
  printf("tried to read %jd actually read %jd\n", size, res);
    //block_fd = open(block_cache_path, O_RDWR | O_CREAT, 0660);
    //if(block_fd < 0) {
    //   printf("unable to open %s\n", block_cache_path);
    //   perror("");
    // }
    // if((res2 = write(block_fd, buffer, res)) < 1) {
    //   printf("error writing to block cache\n");
    //   perror("");
    // } else {
    //   printf("wrote %zd bytes\n", res2);
    // }
  // } else {
  //   printf("file already in cache\n");
  //   block_fd = open(block_cache_path, O_RDONLY);
  //   res = read(block_fd, buffer, size);
  //   printf("sucessfuly read %zd bytes from cache file %s\n", res, block_cache_path);
  //   lseek(backing_fd, res, SEEK_CUR);
  // }

  // close(block_fd);
  // free(phy_block_str);
  // free(block_cache_path);

  return res;
}*/

/*
size_t write_cache_block(cache* file_cache, int backing_fd, const char* buffer, size_t size) {
  int block_fd;
  unsigned long long int phy_block;
  size_t res;
  off_t curr, offset;
  char *phy_block_str, *block_cache_path;

  phy_block_str = NULL;

  curr = lseek(backing_fd, 0, SEEK_CUR);

   printf("fd=%d, block_size=%d, current_offset=%zd\n", backing_fd, file_cache->block_size, curr);
  phy_block = get_physical_block(backing_fd, file_cache->block_size);
  if(phy_block == -1) {
    printf("[fuse] write: write_cache_block: allocating %jd\n", size);
    if(posix_fallocate(backing_fd, curr, size)) {
      perror("");
      return -1;
    }
    phy_block = get_physical_block(backing_fd, file_cache->block_size);
  }

  printf("physical_block=%llu\n", phy_block);

  phy_block_str = physical_block_to_string(phy_block);
  printf("physical_block_str=%s\n", phy_block_str);

 offset = curr % file_cache->block_size;

  +2 because there needs to be room for the extra /
  block_cache_path  = (char*) malloc(sizeof(char) * (strlen(file_cache->cache_dir) + strlen(phy_block_str) + 2));
  strcpy(block_cache_path, file_cache->cache_dir);
  strcat(block_cache_path,PATH_DELIM);
  strcat(block_cache_path,phy_block_str);

  printf("block_path=%s\n", block_cache_path);

  printf("lets write to the cache\n");
  block_fd = open(block_cache_path, O_RDWR | O_CREAT, 0660);
  lseek(block_fd, SEEK_CUR, offset);
  if(block_fd < 0) {
    printf("unable to open %s\n", block_cache_path);
    perror("");
    return -1;
  }
  if((res = write(block_fd, buffer, size)) < 1) {
    printf("error writing to block cache\n");
    perror("");
    return -1;
  } else {
    printf("wrote %zd bytes\n", res);
  }

  if((res = write(backing_fd, buffer, size)) < 1) {
    printf("error writing to block cache\n");
    perror("");
    return -1;
  } else {
    printf("wrote %zd bytes\n", res);
  }
  close(block_fd);
  curr = lseek(backing_fd, res, SEEK_CUR);
  printf("end current %zd\n", curr);
  free(phy_block_str);
  free(block_cache_path);

  return res;
}
*/

/*
int destroy_cache(cache *file_cache) {
  cache_entry *list, *tmp;
  list = file_cache->file_list;
  while(list) {
    tmp=list;
    list=list->next;
    free(tmp->filename);
    free(tmp);
  }

  free(file_cache->cache_dir);
  free(file_cache->backing_dir);

  return 0;
}
*/

/*
void insert_into_cache(cache* file_cache, char* filename) {
  cache_entry *new_entry;
  //int file_key_length;

  //file_key_length = 1;
  //file_key = file_cache->file_key;

  while(file_key > 1) {
    file_key/=10;
    file_key_length++;
  }
  
  
  new_entry = (cache_entry*)malloc(sizeof(cache_entry));
  new_entry->filename=malloc(sizeof(char) * (strlen(filename) + 1));
  strcpy(new_entry->filename,filename);
  //new_entry->file_key = (char*) malloc(sizeof(char) * (file_key_length +1));
  //sprintf(new_entry->file_key,"cached_%d_cached",file_cache->key_generator++);

  / head insert
    recently accessed files are more likely
    to be accessed next, and its simpler to implement)*
  
  new_entry->next = file_cache->file_list;
  file_cache->file_list = new_entry;
}
*/

/*
void display_cache(cache* file_cache) {
  int i;
  cache_entry* entry = file_cache->file_list;
  
  if(entry==NULL) {
    printf("The list is empty\n");
    return;
  }
  for(i=0;entry!=NULL;i++,entry=entry->next) {
    printf("%d: %s\n",i,entry->filename);
  }
}*/

/*
int cache_contains_entry(cache* file_cache, char* filename) {

  cache_entry* entry;
  
  for(entry=file_cache->file_list; entry; entry=entry->next)
    if(!strcmp(entry->filename,filename))
      return 1;
  
  return 0;
}
*/

/*
void delete_cache_entry(cache* file_cache, char* filename) {

  cache_entry *prev,*temp;

  temp = file_cache->file_list;
  prev = file_cache->file_list;;

  if(temp==NULL) {
    printf("The list is empty\n");
    return;
  }
  

  while(temp!=NULL) {
    if(strcmp(temp->filename,filename)==0) {
      if(temp==file_cache->file_list) // First entry
	file_cache->file_list=temp->next; // shifted the header node
      else
	prev->next=temp->next;
      
      free(temp);
      return;
    }
    else {
      prev=temp;
      temp=temp->next; 
    }
  }
}
*/

/*
int write_cache_updates(cache* file_cache) {
  
  cache_entry* aux;
  char* back_path;
  char* cache_path;
  struct stat back_stat, cache_stat;

  
  // can alternately use dirty flag on cache list and set it on every write
  for(aux=file_cache->file_list;aux;aux=aux->next) {
    
    back_path = malloc(sizeof(char) * (strlen(file_cache->backing_dir) + strlen(aux->filename) + 1));
    strcpy(back_path,file_cache->backing_dir);
    strcat(back_path,aux->filename);
    
    cache_path = malloc(sizeof(char) * (strlen(file_cache->cache_dir) + strlen(aux->filename) + 1));
    strcpy(cache_path,file_cache->cache_dir);
    strcat(cache_path,aux->filename);

    if(stat(back_path,&back_stat)) {
      printf("%s doesn't exist\n", back_path);
      return -1;
    }

    if(stat(cache_path,&cache_stat)) {
      printf("%s doesn't exist\n", cache_path);
      return -1;
    }

    if(back_stat.st_mtime < cache_stat.st_mtime) {
      printf("%s: cached file has been modified and needs to be updated\n",aux->filename);
      // should copy whoole file or only modifications
      if(remove(back_path)) {
	printf("unable to remove %s\n",back_path);
	return -1;
      }

      if(copy_file(back_path,cache_path,O_RDWR)) {
	printf("unable to copy cached file %s\n",aux->filename);
	return -1;
      }
    }

    else
      printf("cached file hasn't been modified\n");

    free(back_path);
    free(cache_path);
  }

  return 0;
}
*/

/*
void* cache_loop(void *arg) {
  
  cache_entry* file_cache = (cache_entry*) arg;

  //display_cache(file_cache);
  
  while(1) {
    sleep(3);
    printf("I'm the thread of the world\n");
    //write_cache_updates(file_cache);
  }
}
*/


/*
files are cached on cache root
a key is used as filename on cache folder
*/
/*
int copy_to_cache(char* file_path, cache file_cache) {
  
}
*/

/*
int main() {
  
  cache_entry* file_cache=NULL;
  pthread_t thread;

  append(&file_cache,"/test1");
  append(&file_cache,"/test2");

  /pthread_create(&thread,NULL,cache_loop,(void*)file_cache);
  
  //while(1);
  //  write_cache_updates(file_cache);


  cache cache;
  bzero(&cache, sizeof(cache));
  cache.cache_dir=(char*) malloc(sizeof(char)*(strlen("/home/jdpl/cache")+1));
  cache.backing_dir=(char*) malloc(sizeof(char)*(strlen("/home/jdpl/back")+1));
  strcpy(cache.cache_dir,"/home/jdpl/cache");
  strcpy(cache.backing_dir,"/home/jdpl/back");
  display_cache(&cache);
  load_cache_contents(&cache, cache.cache_dir);
  display_cache(&cache);
  destroy_cache(&cache);
  return 0;
}
*/

