#include<cstdio>
#include<iostream>
#include<string>
#include<vector>
#include<tuple>
#include<fstream>
#include<unordered_map>

using namespace std;
struct Statistics
{
    int circles=0;
    int structure_stalls=0;
    int RAW_stalls=0;
    int WAR_stalls=0;
    int WAW_stalls=0;
}statistics;
struct Ins_State
{
    string raw_ins;
    string ins;
    int rs;
    int rt;
    int rd;
    int imm;
    bool IF;
    bool ID;
    bool EX;
    bool MEM;
    bool WB;
    bool over;
    int res;
};
vector<string> raw_ins_table;
int pc=0;
vector<Ins_State> ins_state_Table;
bool ended=false;
bool redirection=false;
struct Component
{
    string name;
    int busy;
};
vector<Component> component_table={Component{"IF",-1},Component{"ID",-1},Component{"ADD",-1},Component{"MEM",-1}};
unordered_map<int,int> redirection_reg_table={{2,0}};
class Registers
{
    private:
        int content[32];
    public:
        int W[32];
        bool R[32];
        void set(int index,int value)
        {
            if(index!=0)
                content[index]=value;
        }
        int get(int index)
        {
            return content[index];
        }
        Registers()
        {
            fill(content,content+32,0);
            fill(W,W+32,-1);
            fill(R,R+32,false);
        }
}registers;
vector<int> memory;
tuple<string,int,int,int,int> parse(string& s)
{
    try
    {
        size_t b1=s.find(' ');
        size_t comma1=s.find_first_of(',');
        size_t comma2=s.find_last_of(',');
        string ins=s.substr(0,b1);
        string r1=s.substr(b1+1,comma1);
        if(ins.compare("beqz")==0)
        {
            string imm=s.substr(comma1+1,s.length());
            return make_tuple(ins,stoi(r1.substr(2,r1.length())),-1,-1,stoi(imm));
        }
        else if(ins.compare("load")==0)
        {
            string r2=s.substr(comma1+1,s.length());
            return make_tuple(ins,stoi(r2.substr(2,r2.length())),-1,stoi(r1.substr(2,r1.length())),0);
        }
        else if(ins.compare("store")==0)
        {
            string r2=s.substr(comma1+1,s.length());
            return make_tuple(ins,stoi(r1.substr(2,r1.length())),-1,stoi(r2.substr(2,r2.length())),0);
        }
        else if(ins.compare("add")==0)
        {
            string r2=s.substr(comma1+1,comma2);
            string r3=s.substr(comma2+1,s.length());
            return make_tuple(ins,stoi(r2.substr(2,r2.length())),stoi(r3.substr(2,r3.length())),stoi(r1.substr(2,r1.length())),0);
        }
        else
            throw exception();
    }
    catch(const exception e)
    {
        printf("Unknown Instruction: %s\n",s.c_str());
        exit(0);
    }
}
void show_pipelines(void)
{
    printf("\n");
    printf("Pipelines:\n");
    for(int i=0;i<ins_state_Table.size();i++)
    {
        printf("\"%s\": ",ins_state_Table[i].raw_ins.c_str());
        if(ins_state_Table[i].IF)
            printf("IF ");
        if(ins_state_Table[i].ID)
            printf("ID ");
        if(ins_state_Table[i].EX)
            printf("EX ");
        if(ins_state_Table[i].MEM)
            printf("MEM ");
        if(ins_state_Table[i].WB)
            printf("WB ");
        if(ins_state_Table[i].over)
            printf("(end)");
        printf("\n");
    }
    if(ended)
        printf("(All pipelines have ended.)\n");
    printf("\n");
}
void step(void)
{
    ended=true;
    if(ins_state_Table.size()==0)
        ended=false;
    for(int i=0;i<ins_state_Table.size();i++)
        if(!ins_state_Table[i].over)
            ended=false;
    if(ended)
        return;
    statistics.circles++;
    for(int i=0;i<ins_state_Table.size();i++)
    {
        if(ins_state_Table[i].over)
            continue;
        if(ins_state_Table[i].IF==true&&ins_state_Table[i].ID==false&&ins_state_Table[i].EX==false&&ins_state_Table[i].MEM==false&&ins_state_Table[i].WB==false)
        {
            bool flag=false;
            bool conflict=false;
            if(ins_state_Table[i].rs>=0)
            {
                if(redirection)
                {
                    for(int j=0;j<ins_state_Table.size();j++)
                        if(ins_state_Table[j].rd==ins_state_Table[i].rs&&ins_state_Table[j].IF==true&&ins_state_Table[j].ID==true&&ins_state_Table[j].EX==true&&ins_state_Table[j].MEM==false&&ins_state_Table[j].WB==false)
                            ins_state_Table[i].rs=-2;
                }
                else if(registers.W[ins_state_Table[i].rs]!=-1)
                    conflict=true;
            }
            if(ins_state_Table[i].rt>=0)
            {
                if(redirection)
                {
                    for(int j=0;j<ins_state_Table.size();j++)
                        if(ins_state_Table[j].rd==ins_state_Table[i].rt&&ins_state_Table[j].IF==true&&ins_state_Table[j].ID==true&&ins_state_Table[j].EX==true&&ins_state_Table[j].MEM==false&&ins_state_Table[j].WB==false)
                            ins_state_Table[i].rt=-2;
                }
                else if(registers.W[ins_state_Table[i].rt]!=-1)
                    conflict=true;
            }
            if(conflict)
            {
                statistics.RAW_stalls++;
                continue;
            }
            for(int j=0;j<component_table.size();j++)
            {
                if(component_table[j].name.compare("ID")==0&&component_table[j].busy==-1)
                {
                    flag=true;
                    ins_state_Table[i].ID=true;
                    component_table[j].busy=i;
                    if(ins_state_Table[i].rs>=0)
                        registers.R[ins_state_Table[i].rs]=true;
                    if(ins_state_Table[i].rt>=0)
                        registers.R[ins_state_Table[i].rt]=true;
                    if(ins_state_Table[i].rd>=0)
                        registers.W[ins_state_Table[i].rd]=i;
                    for(int k=0;k<component_table.size();k++)
                        if(component_table[k].name.compare("IF")==0&&component_table[k].busy==i)
                            component_table[k].busy=-1;
                    break;
                }
            }
            if(flag==false)
                statistics.structure_stalls++;
        }
        else if(ins_state_Table[i].IF==true&&ins_state_Table[i].ID==true&&ins_state_Table[i].EX==false&&ins_state_Table[i].MEM==false&&ins_state_Table[i].WB==false)
        {
            if(ins_state_Table[i].ins.compare("add")==0)
            {
                bool flag=false;
                for(int j=0;j<component_table.size();j++)
                    if(component_table[j].name.compare("ADD")==0&&component_table[j].busy==-1)
                    {
                        flag=true;
                        if(redirection)
                        {
                            if(ins_state_Table[i].rs==-2)
                                ins_state_Table[i].res=redirection_reg_table[j]+registers.get(ins_state_Table[i].rt);
                            else if(ins_state_Table[i].rt==-2)
                                ins_state_Table[i].res=registers.get(ins_state_Table[i].rs)+redirection_reg_table[j];
                            else
                                ins_state_Table[i].res=registers.get(ins_state_Table[i].rs)+registers.get(ins_state_Table[i].rt);
                        }
                        else
                            ins_state_Table[i].res=registers.get(ins_state_Table[i].rs)+registers.get(ins_state_Table[i].rt);
                        component_table[j].busy=i;
                        if(redirection)
                            redirection_reg_table[j]=ins_state_Table[i].res;
                        break;
                    }
                if(flag==false)
                {
                    statistics.structure_stalls++;
                    continue;
                }
            }
            else if(ins_state_Table[i].ins.compare("beqz")==0)
            {
                if(registers.get(ins_state_Table[i].rs)==0)
                {
                    pc=ins_state_Table[i].imm;
                    while(i+1!=ins_state_Table.size())
                        ins_state_Table.pop_back();
                    for(int k=0;k<component_table.size();k++)
                        if(component_table[k].name.compare("IF")==0||component_table[k].name.compare("ID")==0)
                            component_table[k].busy=-1;
                }
            }
            ins_state_Table[i].EX=true;
            registers.R[ins_state_Table[i].rs]=false;
            registers.R[ins_state_Table[i].rt]=false;
            for(int j=0;j<component_table.size();j++)
                if(component_table[j].name.compare("ID")==0&&component_table[j].busy==i)
                    component_table[j].busy=-1;
        }
        else if(ins_state_Table[i].IF==true&&ins_state_Table[i].ID==true&&ins_state_Table[i].EX==true&&ins_state_Table[i].MEM==false&&ins_state_Table[i].WB==false)
        {
            bool flag=false;
            for(int j=0;j<component_table.size();j++)
            {
                if(component_table[j].name.compare("MEM")==0&&component_table[j].busy==-1)
                {
                    component_table[j].busy=i;
                    ins_state_Table[i].MEM=true;
                    flag=true;
                }
            }
            if(flag==false)
            {
                statistics.structure_stalls++;
                continue;
            }
            if(ins_state_Table[i].ins.compare("load")==0)
                registers.set(ins_state_Table[i].rd,memory[registers.get(ins_state_Table[i].rs)]);
            else if(ins_state_Table[i].ins.compare("store")==0)
                memory[registers.get(ins_state_Table[i].rd)]=registers.get(ins_state_Table[i].rs);
            if(ins_state_Table[i].ins.compare("add")==0)
                for(int j=0;j<component_table.size();j++)
                    if(component_table[j].name.compare("ADD")==0&&component_table[j].busy==i)
                        component_table[j].busy=-1;
        }
        else if(ins_state_Table[i].IF==true&&ins_state_Table[i].ID==true&&ins_state_Table[i].EX==true&&ins_state_Table[i].MEM==true&&ins_state_Table[i].WB==false)
        {
            for(int j=0;j<component_table.size();j++)
                if(component_table[j].name.compare("MEM")==0&&component_table[j].busy==i)
                    component_table[j].busy=-1;
            if(ins_state_Table[i].rd>=0&&registers.R[ins_state_Table[i].rd])
            {
                statistics.WAR_stalls++;
                continue;
            }
            if(!redirection)
                if(ins_state_Table[i].rd>=0&&registers.W[ins_state_Table[i].rd]!=-1&&registers.W[ins_state_Table[i].rd]<i&&ins_state_Table[registers.W[ins_state_Table[i].rd]].rs!=-2&&ins_state_Table[registers.W[ins_state_Table[i].rd]].rt!=-2)
                {
                    statistics.WAW_stalls++;
                    continue;
                }
            ins_state_Table[i].WB=true;
            if(ins_state_Table[i].ins.compare("add")==0)
                registers.set(ins_state_Table[i].rd,ins_state_Table[i].res);
            for(int j=0;j<component_table.size();j++)
                if(component_table[j].name.compare("MEM")==0&&component_table[j].busy==i)
                    component_table[j].busy=-1;
        }
        else if(ins_state_Table[i].IF==true&&ins_state_Table[i].ID==true&&ins_state_Table[i].EX==true&&ins_state_Table[i].MEM==true&&ins_state_Table[i].WB==true)
        {
            registers.W[ins_state_Table[i].rd]=-1;
            ins_state_Table[i].over=true;
        }
        else
            printf("Pipeline Error\n");
    }
    if(pc<raw_ins_table.size())
    {
        for(int i=0;i<component_table.size();i++)
            if(component_table[i].name.compare("IF")==0&&component_table[i].busy==-1)
            {
                string ins;
                int rs,rt,rd,imm;
                tie(ins,rs,rt,rd,imm)=parse(raw_ins_table[pc]);
                component_table[i].busy=ins_state_Table.size();
                ins_state_Table.emplace_back(Ins_State{raw_ins_table[pc],ins,rs,rt,rd,imm,true,false,false,false,false,false,0});
                pc++;
            }
    }
}
void show_regs(void)
{
    printf("\n");
    printf("Registers:\n");
    for(int i=0;i<32;i++)
    {
        printf("R%d:%d",i,registers.get(i));
        if(registers.R[i])
            printf(" R");
        if(registers.W[i]!=-1)
            printf(" W");
        printf("\n");
    }
    printf("\n");
}
void show_components(void)
{
    printf("\n");
    printf("Components:\n");
    for(int i=0;i<component_table.size();i++)
    {
        printf("%s:",component_table[i].name.c_str());
        if(component_table[i].busy==-1)
            printf("free");
        else
            printf("busy");
        printf("\n");
    }
    printf("\n");
}
void show_statistics(void)
{
    printf("\n");
    printf("Statistics:\n");
    printf("Total Circles:%d\n",statistics.circles);
    printf("Structure Stalls:%d\n",statistics.structure_stalls);
    printf("RAW Stalls:%d\n",statistics.RAW_stalls);
    printf("WAR Stalls:%d\n",statistics.WAR_stalls);
    printf("WAW Stalls:%d\n",statistics.WAW_stalls);
    printf("\n");
}
int main(int argc, char** argv)
{
    if(argc!=3&&argc!=4){
        printf("Usage: MIPSsim codefilename memoryfilename (redirection)\n");
        return 0;
    }
    if(argc==4&&strcmp(argv[3],"redirection")!=0)
    {
        printf("Usage: MIPSsim codefilename memoryfilename (redirection)\n");
        return 0;
    }
    if(argc==4&&strcmp(argv[3],"redirection")==0)
    {
        redirection=true;
        printf("Redirection enabled\n");
    }
    ifstream file(argv[1]);
    if(!file.is_open()){
        printf("Can't open file %s\n",argv[1]);
        return 0;
    }
    string s;
    while(getline(file,s))
        raw_ins_table.emplace_back(s);
    file.close();
    ifstream mem(argv[2]);
    if(!mem.is_open()){
        printf("Can't open file %s\n",argv[2]);
        return 0;
    }
    int num;
    while(mem>>num)
        memory.emplace_back(num);
    mem.close();
    while(true)
    {
        string input;
        printf(">>>");
        cin>>input;
        if(input.compare("exit")==0)
            break;
        else if(input.compare("n")==0||input.compare("next")==0)
        {
            step();
            show_pipelines();
        }
        else if(input.compare("r")==0||input.compare("regs")==0) 
            show_regs();
        else if(input.compare("c")==0||input.compare("components")==0) 
            show_components();
        else if(input.compare("b")==0||input.compare("break")==0) 
        {
            int num;
            cin>>num;
            while(ins_state_Table.size()<num+1&&!ended)
                step();
            show_pipelines();
        }
        else if(input.compare("s")==0||input.compare("statistics")==0)
            show_statistics();
        else
            printf("Unknown command: %s\n",input.c_str());
    }
    return 0;
}