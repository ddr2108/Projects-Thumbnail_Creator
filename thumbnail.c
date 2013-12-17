#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

// portions derived from IJG code
#define readbyte(a,b) 	do if(((a)=getc((b))) == EOF) return 0; while (0)
#define readword(a,b) 	do { int cc_=0,dd_=0; \
                        	if((cc_=getc((b))) == EOF \
        		  			|| (dd_=getc((b))) == EOF) return 0; \
                          	(a) = (cc_<<8) + (dd_); \
                      	} while(0)

//Max image size
#define MAX_IMAGE_SIZE 3000*2000

//Functional Prototypes
int getImageSize (char* image, int* width, int* height);
void readDirectory(char* cwd);
void shrinkImage(char* cwd, char* image);
void scaleImage(char* inImageName, char* outImageName, int width, int height, double scale);

//////////////////
//Get Image Size
//////////////////
int getImageSize (char* inImageName, int* width, int* height) {
	FILE* image;
	int dummy=0;
   	int discarded_bytes=0;
	int length = 0;
	int marker=0;

	image = fopen(inImageName, "rb");					//Open file

  	//Check if first byte is correct
  	if (getc(image) != 0xFF || getc(image) != 0xD8)
    	return 0;

  	while (1) {
   		
   		//Read in bytes till get to 0xFF
    	readbyte(marker,image);
    	while (marker != 0xFF) {
      		discarded_bytes++;
      		readbyte(marker,image);
    	}
    	//Read till no more 0xFF
    	do readbyte(marker,image); 
    	while (marker == 0xFF);
    	//return failure if no header
    	if (discarded_bytes != 0){ 
    		fclose(image);
    		return 0;
    	}
   
   		//Use marker to get info
	    switch (marker) {
		    case 0xC0:
		    case 0xC1:
		    case 0xC2:
		    case 0xC3:
		    case 0xC5:
		    case 0xC6:
		    case 0xC7:
		    case 0xC9:
		    case 0xCA:
		    case 0xCB:
		    case 0xCD:
		    case 0xCE:
		    case 0xCF:
		    	//Read data till get to size data
			    readword(dummy,image);
			    readbyte(dummy,image);
			    readword((*height),image);
			    readword((*width),image);
			    readbyte(dummy,image);
			    fclose(image);
			    return 1;
	    	case 0xDA:
	    	case 0xD9:
	      		return 0;
	    	default: 
		
				//Read in length
				readword(length,image);
				if (length < 2){
					fclose(image);
				  	return 0;
				}
				//Read the length bytes and continue
				length -= 2;
				while (length > 0) {
				 	readbyte(dummy, image);
				  	length--;
				}
			    
			    break;
		}
  }
  fclose(image);
}

//////////////////
//Read Directory
//////////////////
void readDirectory(char* cwd){    
    DIR *dir;				//struct to hold directory
    struct dirent *dent;	//struct to hold dents
	char newFileName[1024];		//buffer for new directories

	//Open directory
	dir = opendir(cwd);		

	//Read in all files in directory
	while ((dent = readdir(dir)) != NULL){
		if (strstr(dent->d_name,"thumbnail")!=NULL){
			continue;
		}else if (strstr(dent->d_name,"jpg")!=NULL){
			//Create new image name
			strcpy(newFileName, cwd);
			strcat(newFileName, "/");
			strcat(newFileName, dent->d_name);

			shrinkImage(newFileName, dent->d_name);
		}else if(strstr(dent->d_name,".")!=NULL){
			continue;
		}else{
			//Create new directory name
			strcpy(newFileName, cwd);
			strcat(newFileName, "/");
			strcat(newFileName, dent->d_name);

			//Recursively access directories
			readDirectory(newFileName);
		}
	}

	//Close directory
	(void)closedir(dir);

}

//////////////////
//Shrink Image
//////////////////
void shrinkImage(char* cwd, char* imageName){
	char newImageName[1024];
	int height, width;
	int newHeight, newWidth;
	double scale;
	int i = 0;

	getImageSize(cwd, &width, &height);		//Get image size

	//Set so shorter side is always 150px
	if (width>height){
		newHeight = 150;
		scale = (double)height/newHeight;
		newWidth = width/scale;
	}else if(height>width){
		newWidth = 150;
		scale = (double)width/newWidth;
		newHeight = height/scale;
	}else{
		scale = 1;
		newHeight = 150;
		newWidth = 150;
	}

	//Create new image name
	while(cwd[i]!='.'){
		newImageName[i] = cwd[i];
		i++;
	}
	newImageName[i] = '\0';
	strcat(newImageName, "-thumbnail.jpg");

	//Scale image
	scaleImage(cwd, newImageName, width, height, scale);	
}

//////////////////
//Scale Image
//////////////////
void scaleImage(char* inImageName, char* outImageName, int width, int height, double scale){
	int newIndex;
	int x,y;
	unsigned char inImageData[MAX_IMAGE_SIZE];
	FILE* inImage,* outImage;

	//Read image into buffer
	inImage = fopen(inImageName, "rb");
	fread(inImageData, MAX_IMAGE_SIZE, 1, inImage);
	fclose(inImage);

	//Open file
	outImage = fopen(outImageName, "wb");					
	//Start Scaling	
	for (y = 0; y<width/scale; y++){
		for (x = 0; x<height/scale; x++){
			newIndex = y*width + (int)(x*scale);
			fwrite(inImageData+newIndex,sizeof(char), 1, outImage);	
		}
		fwrite("\n", sizeof(char), 1, outImage);
	}
	//Close File
	fclose(outImage);

}

int main(){
	char cwd[1024];			//Buffer for cwd
	
	//Get the current directory and read it
	getcwd(cwd, sizeof(cwd));
	readDirectory(cwd);
}
