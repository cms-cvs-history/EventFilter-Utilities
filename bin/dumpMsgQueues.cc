#include <stdio.h>       /* standard I/O functions.              */
#include <stdlib.h>      /* malloc(), free() etc.                */
#include <sys/types.h>   /* various type definitions.            */
#include <sys/ipc.h>     /* general SysV IPC structures          */
#include <sys/msg.h>     /* message queue functions and structs. */
#include <errno.h>
#include <string.h>
#include <vector>
#include <iostream>

#include "EventFilter/Utilities/interface/Exception.h"
#include "EventFilter/Utilities/interface/MsgBuf.h"
#include "EventFilter/Utilities/interface/SlaveQueue.h"


int main(){

  unsigned int monitor_offset = 200;
  unsigned int i = 1;
  std::vector<evf::SlaveQueue *> qq;
  std::vector<evf::SlaveQueue *> mq;
  try{
    while(1){
      std::cout << "trying to get msg queue " << i << std::endl;
      evf::SlaveQueue *sqq = new evf::SlaveQueue(i);
      qq.push_back(sqq);
      evf::SlaveQueue *smq = new evf::SlaveQueue(i+ monitor_offset);
      mq.push_back(smq);
      i++;
    }
  }
  catch(evf::Exception &e)
    {
      std::cout << "Finished with exception " << e.what() << std::endl;
    }

  for(i = 0; i < qq.size(); i++)
    {
      evf::MsgBuf buf;
      int retval = qq[i]->rcvNonBlockingAny(buf);
      std::cout << "from queue " << qq[i]->id() << " returned " << retval 
		<<" received type " 
		<< buf->mtype << std::endl;
      evf::MsgBuf buf1;
      retval = mq[i]->rcvNonBlockingAny(buf1);
      std::cout << "from mon queue " << mq[i]->id() << " returned " << retval 
		<< " received type " 
		<< buf1->mtype << std::endl;

    }

}
