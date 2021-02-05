
#include <iostream>
#include <string>
#include <string.h> //strcpy
#include <vector>
#include <sstream>
#include <unistd.h> //execv
#include <sys/wait.h> // waitpid
#include <algorithm>
#include <dirent.h>

/*using namespace std;
盡量避免使用 using namespace std; 等直接引入整個命名空間，否則會因為命名空間污染導致很多不必要的問題， 比如自己寫的某個函數，名稱正好和 std 中的一樣， 編譯器會不知道使用哪一個， 引起編譯報錯*/
using std::cin;
using std::cout;
using std::string;
using std::vector;
using std::istringstream;


vector<string> sh_getCmds(int &pipe_num)
{
    string line;
    string token;
    vector<string> cmds;
    getline(cin, line);

    
    istringstream ss(line);               //將打好的東西放到字串delim裡,包含空白
    while(getline(ss,token,'|'))           //getline(delim[來源位置],token[存入位置],'　'[分割的條件])
    {
        if(token != "")
  	{
            cmds.push_back(token);                
	}
    }
    
    return cmds;
}


bool IsBuildIn(string cmd)
{
    if(cmd == "printenv" || cmd == "setenv" || cmd == "exit")
        return true;
    return false;
}

bool isCmd(string cmd)
{
    DIR *dir; struct dirent *diread;
    vector<string> files;
    bool res = false;
    if ((dir = opendir("./bin/")) != nullptr) 
    {
        while ((diread = readdir(dir)) != nullptr) 
        {
            if(diread->d_name == cmd)
	    {
		res =  true;
                break;
	    }
        }
        closedir (dir);
    } 
    else 
    {
        perror ("opendir");
        return EXIT_FAILURE;
    }

    return res;
}
void parseCmd(vector<string>::iterator &it, vector<string> &args, string &cmd)
{
    string token;
    bool first = true;
    istringstream ss(*it);               //將打好的東西放到字串delim裡,包含空白
    while(getline(ss,token,' '))           //getline(delim[來源位置],token[存入位置],'　'[分割的條件])
    {
        if(token != "")
  	{
            if(first)
            { 
  	        first = false;
		cmd = token;
	    }
	    else
                args.push_back(token);                

	}
    }

}

int sh_runCmd(int pipefds[], int &count, int &lastCmd, int fdsize, vector<string>::iterator &it)
{
    int status;
    pid_t pid;
    vector<string> args;
    string cmd;
    parseCmd(it, args, cmd); 
    if(IsBuildIn(cmd))
    	{ 
            string path;
            if(cmd == "printenv")
            { 
                if (args.size() >= 1)
                    path = getenv(args[0].c_str());
                cout << path << "\n";
                return 1;
            }
            else if(cmd == "setenv")
            {
                setenv(args[0].c_str(), args[1].c_str(), 1);
		return 1;
            } 
            else
                return 1;
    }
    if(!isCmd(cmd)) 
    {
	cout << "Unknown command: [" << cmd  << "].\n";
        return 1;
    }
    pid = fork();
    if(pid == 0)
    { 
        if ( std::find( args.begin( ), args.end( ), ">" ) != args.end() ) 
        {
	     
             string targetfile = args[args.size()-1];
             FILE * fp;
             fp = fopen(targetfile.c_str(), "w");
             if(fp == NULL)
             {
                 perror("Open file error!");
             	 return 0;
             }
             close(pipefds[0]);
             pipefds[1] = fileno(fp);	
             dup2(pipefds[1], 1);
             close(pipefds[1]);
             args.pop_back();
             args.pop_back();
        }
        else if(count != lastCmd-1)
        {
            close(pipefds[0]);
	    if( dup2(pipefds[1], 1) < 0 ){
                perror("pipe error");
       		return 0;
            }
            close(pipefds[1]);
        }
        
        if(count != 0)	
        {
            close(pipefds[1]); 
            dup2(pipefds[0], 0); 
            close(pipefds[0]);  
        }
        //exec
        char *argv[args.size()+2]; // one extra for the cmd, another for the null
        argv[0] = &cmd[0];
        for(size_t i = 0; i < args.size(); i++)
        {
            argv[i+1] = &args[i][0];
        }
        argv[args.size()+1] = NULL;
        string path = "./bin/" + cmd;

        if (execv(path.c_str(), argv) == -1) {
            perror("sh_error");
            return 0;
        }
        
    } 
    else if (pid < 0) {
        // Fork 出错
        perror("fork error");
	return 0;
    } 
    else
    {
 	wait(NULL); 		
	close(pipefds[1]); //why
    }
}

void sh_loop()
{ 
    
    vector<string> cmds;
    vector<string> args;
    string cmd;
    int sign = 0;
    int status;
    int isExit = 1;
    int pipe_num = 0;
    bool haspipe = false;
    vector<string>::iterator end;
    vector<string>::iterator it;

    do{   
        cout << "% "; 
        cmds = sh_getCmds(pipe_num); 
	it = cmds.begin();
        end = cmds.end();
        int pipefds[2];
	
    if(pipe(pipefds) < 0)
    {
        perror("pipe error");
        return ;
    }
        int count = 0, lastCmd = cmds.size();

        while(it != end )
        {
	    isExit = sh_runCmd(pipefds, count, lastCmd, pipe_num*2, it);

count++;
            it++;
            /*
            parseCmd(it, end, sign, cmd, args, haspipe);
            if(sign == 1)
            {
  	    	string targetfile = args[args.size()-1];
                FILE * fp;
            	fp = fopen(targetfile.c_str(), "w");
            	if(fp == NULL)
                {
            	    perror("Open file error!");
             	    return;
            	}
            	pipefd[1] = fileno(fp);
            	args.pop_back();
	    }
            if(sign == 2)
            {
  	        
	    }  
            status = sh_runCmd(pipefd[0], pipefd[1], sign, cmd, args, haspipe);
                
    	    if(sign == 2)
   	    { 
                haspipe = true;
            } 
            sign = 0; 
            line_split.clear();
            args.clear();    */
        }

    }while(isExit);
}
        

int main() {
    setenv("PATH", "bin:.", 1);
    sh_loop(); 
}
