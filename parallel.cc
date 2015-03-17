// --*-c++-*--

#include <stdlib.h>
#include <mpi.h>

#include <iostream>
#include <string>

#include "image.h"
#include "image_utils.h"

std::string parallel_main(int argc, char* argv[])
{
  if (argc < 5) {
    std::cout << "usage: " << argv[0] << " "
	      << "<in image> <out image> <filter type> <window size>"
              << std::endl;
    std::cout << " filter type = {mean|median} " << std::endl;
    exit(-1);
  }

  int rank, namelen,count;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name(processor_name, &namelen);
  MPI_Comm_size(MPI_COMM_WORLD,&count);

  std::string inFilename(argv[1]);
  std::string outFilename(argv[2]);
  std::string filterType(argv[3]);
  int windowSize = atoi(argv[4]);

  outFilename = "p_" + outFilename;

  Image input;

  //input for threads
  Image portion;
  int width;
  int height;
  uint32_t *image;
  MPI_Status status;
  int edge = windowSize/2;



    // std::cout << "* doing parallel image filtering ** with "<<count<<" threads on "<<rank << std::endl;
  if (rank == 0) {
    std::cout << "* doing parallel image filtering ** "<<std::endl;//with "<<count<<" threads  " << std::endl;
    std::cout << "* reading regular tiff: " << argv[1] << std::endl;
    input.load_tiff(std::string(argv[1]));
    // portion.buildFromArray(input.getArray(),input.width(),input.height()/count);
    
    input.make_greyscale();
    int pace = input.width()*(input.height()/count);
    int tempPace = pace + edge*input.width();
    int startPoint=0;
    int size = input.width()*input.height();
    int tempWidth=input.width(),tempHeight;
    // std::cout<<"INPUT H&W "<<input.width()<<"--"<<input.height()<<std::endl;
    for ( int i = 1 ; i < count ; i++){
      startPoint = i*pace;//input.width()*input.height()/count;
      // startPoint = (startPoint- edge*input.width() > 0 )? startPoint - edge*input.width() : startPoint;
      startPoint = startPoint - edge*input.width();
      if( i == count -1 )
        tempHeight = input.height()/count + edge + (input.height()%(input.height()/count));
      else
        tempHeight = input.height()/count + edge*2; 
      //tempHeight = ((startPoint- edge*input.width() < 0 )?0:edge)+      
       // startPoint+tempPace < size ? (input.height()/count)+edge:  (startPoint+pace < size?(input.height()/count): (size-startPoint)/input.width());
      MPI_Send(&tempWidth, 1 , MPI_INT, i,0,MPI_COMM_WORLD);
      MPI_Send(&tempHeight, 1 , MPI_INT , i , 0 , MPI_COMM_WORLD);
      // std::cout<<"sending H&W H&W "<<input.width()<<"--"<<tempHeight<<std::endl;
      // if( i == count -1 )
         // MPI_Send(input.getArray()+startPoint, tempWidth*);
      MPI_Send(input.getArray()+startPoint,tempHeight*tempWidth 
        ,MPI_INT, i,0,MPI_COMM_WORLD);
      //MPI_Send(&i,1, MPI_INT,i,0,MPI_COMM_WORLD);
      // std::cout<<"sent everything on parent node--"<<i<<std::endl;
      
    }
      // startPoint = i*input.width()*input.height()/count;
      tempHeight = ((tempPace < size) ? (input.height()/count)+edge:  (pace < size?(input.height()/count): (size)/input.width()));
      portion.buildFromArray(input.getArray(),input.width(),tempHeight);

  }else{
    // MPI_Recv(&width,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&width,1,MPI_INT, 0,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&height,1,MPI_INT, 0,0,MPI_COMM_WORLD,&status);
     // std::cout<<"receiving on   --"<<width<<" as width "<<height<<" as heigt"<<std::endl;
    int * tempInt = new int[height*width];
    // image = new uint32_t[height*width];
    MPI_Recv(tempInt,width*height,MPI_INT, 0,0,MPI_COMM_WORLD,&status);
    image = (uint32_t *)tempInt;
    // std::cout<<"received the array"<<std::endl;
    portion.buildFromArray(image,width,height);
    //if( rank == 1)
      // portion.save_tiff_grey_8bit("firstImage");


  }

  Image output ;
  std::cout << "[" << processor_name << "] processing with filter: " 
            << filterType << std::endl;

  if (filterType == "mean") {

    // begin parallel code
    portion.image_filter_mean(windowSize); 
    // end parallel code
  }
  else if (filterType == "median") {
    // begin parallel code

    portion.image_filter_median(windowSize);
    // end parallel code
  }

  if (rank == 0) {
    // std::cout<<"Mother process gathering the data"<<std::endl;
    uint32_t * temp ;
    for(int i = 1 ; i < count ; i++){
      // std::cout<<"Process "<<i<<std::endl;
      //if( i < count-1){

      //}
      MPI_Recv(&width,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
      MPI_Recv(&height,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
      int * tempInt = new int[height*width];
      temp = new uint32_t[height*width];
      MPI_Recv(tempInt,width*height,MPI_INT,i,0,MPI_COMM_WORLD,&status);
      temp = (uint32_t *)tempInt;
      Image tempImage;
      tempImage.buildFromArray(temp+(edge*width),width,height);
      portion.mergeVerticals(temp,width,height,edge);
      //if( i == 1)
        //tempImage.save_tiff_grey_8bit("tempSave.tiff");
      // tempImage.save_tiff_grey_8bit("ff.tiff");
      
      //std::cout<<"received  "<<t<<std::endl;
    }
    std::cout << "[" << processor_name << "] saving image: " 
              << outFilename 
              << std::endl;
    std::cout<<"end "<<portion.width()<<"----"<<portion.height()<<std::endl;
    output.buildFromArray(portion.getArray(),portion.width(),portion.height());
    // output.save_tiff_grey_8bit(outFilename);
    output.save_tiff_grey_32bit(outFilename);
  }else{
      if( rank == 1)
        portion.save_tiff_grey_8bit("fa");
      MPI_Send(&portion.width(),1,MPI_INT,0,0,MPI_COMM_WORLD);
      MPI_Send(&portion.height(),1,MPI_INT,0,0,MPI_COMM_WORLD);
      MPI_Send(portion.getArray(),portion.width()*portion.height(),MPI_INT,0,0,MPI_COMM_WORLD);
  }

  std::cout << "-- done --" << std::endl;

  return outFilename;
}
