/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name: Robert Chu	
	UIN: 418009107
	Date: 9/17/2024
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	int buffercapacity = MAX_MESSAGE;// added optional arguement m, default set to 5000
	string filename = "1.csv";

	//Add other arguments here
	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {// added m:
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				buffercapacity = atoi (optarg);
				break;
		}
	}

	//Task 1:
	//Run the server process as a child of the client process


	string string_m = to_string(buffercapacity);
	char const * m_char = string_m.c_str();
	pid_t clientPid = fork();//make a duplicate process


	// system(string_p.c_str());

    char* clientArgs[] = {(char*) "./server", (char*) "-m", (char*) m_char, nullptr};
	if(clientPid == 0)//must be child
	{
		execvp(clientArgs[0], clientArgs);//run the server as child process
	}


    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	//Task 4:
	//Request a new channel
	MESSAGE_TYPE channelRequest = NEWCHANNEL_MSG;
	chan.cwrite(&channelRequest, sizeof(MESSAGE_TYPE));

	char channelName[30];
	chan.cread(&channelName, sizeof(char)*30);//read the new channel name that was sent by server
	string channelNameString(channelName);
    FIFORequestChannel newChannel(channelName, FIFORequestChannel::CLIENT_SIDE);
//	cout << channelName << channelName[6] << endl;
//	cout << "done getting channel name" << endl;	
	//Task 2:
	//Request data points
    char buf[MAX_MESSAGE];//create buffer that can be used to read and write data
    datamsg x(p, t, e);

	memcpy(buf, &x, sizeof(datamsg)); //copy the datamsg struct to buffer
	chan.cwrite(buf, sizeof(datamsg));
	double reply;// this is where the server's response (ecg value) is stored after reading from named pipe
	chan.cread(&reply, sizeof(double));
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

	string x1_name = "x1.csv";
	string x1_filepath = "received/" + x1_name; 
	// FILE* x1_file = fopen(x1_filepath.c_str(), "wb");
	std::ofstream fout(x1_filepath);// using ofstream instead of FILE* to concatenate data easier
	double ecg1;//where ecg1 data is stored
	double ecg2;//where ecg2 data is stored
	for (int step = 0; step < 1000; ++step)
	{
		//look at ecg1
		x = datamsg(p,0.004*step,1);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));//could also put x here
		chan.cread(&ecg1, sizeof(double));

		//look at ecg2
		x = datamsg(p,0.004*step,2);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		chan.cread(&ecg2, sizeof(double));

		fout << step*0.004 << "," << ecg1 << "," << ecg2 << endl;//format the data like how it is in original file
		// output stream is better since below would be more complicated to implement
		// memcpy(&x1_buffer, &t, sizeof(double));
		// memcpy(&x1_buffer+sizeof(double), (char *) ',', sizeof(char));
		// memcpy(&x1_buffer+sizeof(double)+sizeof(char), &ecg1)
		// to_string(ecg1) + "," to_string(ecg2)

	}
	fout.close();

	
	//Task 3:
	//Request files
	filemsg fm(0, 0);//client asking for file size from server

	string fname = filename; //filename from input
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];//create buffer for file requests
	memcpy(buf2, &fm, sizeof(filemsg));//copies file message to buf2
	strcpy(buf2 + sizeof(filemsg), fname.c_str());//copies string to memory address after filemsg object
	chan.cwrite(buf2, len); //request file data

	__int64_t file_length;
	chan.cread(&file_length, sizeof(__int64_t));
	cout << "The length of " << fname << " is " << file_length << endl;

	
	filemsg fm_getData(0,0);//placeholder object, will be replaced later in for loop to get data of a certain size at offset specified
	char* fileData = new char[buffercapacity];

	string filepath = "received/" + fname; 
	FILE* fp = fopen(filepath.c_str(), "wb");//open file up for writing
	if (!fp) {
	cerr << "Client cannot open file: " << filepath << endl;
	}
	int read_size = buffercapacity;//by default, read size is buffer capacity. It will change if the data left in file is less than buffer capacity
	for(int offset = 0; offset < file_length; offset=offset + buffercapacity)
	{
		if (!(offset <= file_length - buffercapacity))
		{
			read_size = file_length - offset;
		}
		cout << "off" << offset << " " << read_size << endl;
		fm_getData = filemsg(offset,read_size);
		memcpy(buf2, &fm_getData, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		// cout << "1" << endl;
		chan.cwrite(buf2,len);
		chan.cread(fileData, buffercapacity);

//		for (int i = 0; i<buffercapacity; ++i)//print out data
//		{
//			cout << "filedata" << fileData[i] <<endl;
//		}

		fwrite(fileData, 1, read_size, fp);
	}

	fclose(fp);

	delete[] fileData;
	delete[] buf2;
	
	//Task 5:
	// Closing all the channels
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	newChannel.cwrite(&m, sizeof(MESSAGE_TYPE));
}
