#include "DirManager.h"

namespace evf{

  unsigned int DirManager::findHighestRun(){
    DIR *dir = opendir(dir_.c_str());
      struct dirent *buf;
      int maxrun = 0;
      while((buf=readdir(dir))){
	if(atoi(buf->d_name) > maxrun){maxrun = atoi(buf->d_name);}
      }
      closedir(dir);
      return maxrun;
  }
  std::string DirManager::findHighestRunDir(){
    std::string retval = dir_ + "/";
    std::string tmpdir;
    DIR *dir = opendir(dir_.c_str());
    struct dirent *buf;
    int maxrun = 0;
    while((buf=readdir(dir))){
      if(atoi(buf->d_name) > maxrun){tmpdir = buf->d_name; maxrun = atoi(buf->d_name);}
    }
    closedir(dir);
    retval += tmpdir;
    return retval;
  }
  
  bool DirManager::checkDirEmpty(std::string &d)
  {

    int filecount=0;
    DIR *dir = opendir(d.c_str());
    struct dirent *buf;
    while((buf=readdir(dir))){
      filecount++;
    }
    return (filecount==0);
  }
}
