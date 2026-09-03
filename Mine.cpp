#include<iostream>
#include<vector>
#include<random>
#include<fstream>
#include<string>
#include<cstdlib>
#include<chrono>
#include<mutex>
#include"conio.hpp"

//27=ESC 
const char *unveal="\033[44;32m";
const char *veal="\033[0m";

const char *unveal_focus="\033[34;42m";
const char *veal_focus="\033[30;47m";

const char *normal="[]";
const char *doubt="??";
const char *label="!!";
const char *boom="@@";
const char *veal_number[]={"  "," 1"," 2"," 3"," 4"," 5"," 6"," 7"," 8"};

std::string const chosen=std::string("\033[47;30m");
std::string const unchosen=std::string("\033[0m");
enum class state_t{normal=0,doubt=2,label=1};

Conioer conin;

struct point{
	int x,y;
};
bool ask_YN(const std::string &question){
	std::cout<<question<<"(Y/N):Y"<<std::flush;
	bool ok_no{true};
	while(1){
		switch(conin.getch()){
		case 'Y':
		case 'y':
			ok_no=true;
			std::cout<<"\bY"<<std::flush;
			break;
		case 'n':
		case 'N':
			ok_no=false;
			std::cout<<"\bN"<<std::flush;
			break;
		case '\r':
			return ok_no;
		}
	}
}
class blocks{
protected:
	int value;
	bool if_reveal;
	bool if_focus;
	state_t state;
	bool if_boom;
public:
	blocks():value(0),if_reveal(false),if_focus(false),state(state_t::normal),if_boom(false){};
	blocks(const blocks&)=default;
	void read(std::ifstream&);
	void save(std::ofstream&);
	void print();
	bool Q_button();
	bool try_reveal();
	bool is_reveal(){return if_reveal;}
	bool is_boom(){return if_boom;}
	void set_boom(){if_boom=true;}
	void set_focus(){if_focus=true;}
	void loss_focus(){if_focus=false;}
	void add_value(){++value;}
	void E_button();
	int get_value() const {return value;}
	state_t get_state() const {return state;}
};
void blocks::save(std::ofstream&ofs){
	ofs.put(value|(static_cast<int>(state)<<4)|(static_cast<int>(if_reveal)<<6)|(static_cast<int>(if_boom)<<7));
}
void blocks::read(std::ifstream&ifs){
	auto getin= ifs.get();
	if (getin=='\n') throw std::runtime_error("end");
	value = getin&0xF;
	state=static_cast<state_t>((getin>>4)&0x3);
	if_reveal=getin&(1<<6);
	if_boom=getin&(1<<7);
}
bool blocks::try_reveal(){
	if(state==state_t::label){
		return false;
	}
	if(if_reveal){
		return false;
	}
	if_reveal=true;
	if(value==0){
		return true;
	}else{
		return false;
	}
}

void blocks::E_button() {
    state = static_cast<state_t>((static_cast<int>(state) + 1) % 3);
}

void blocks::print(){
	if(if_reveal){
		std::cout<<(if_focus?veal_focus:veal);
		if(if_boom){
			std::cout<<boom;
		}else{
			std::cout<<veal_number[value];
		}
	}else{
		std::cout<<(if_focus?unveal_focus:unveal);
		switch(state){
			case state_t::normal:std::cout<<normal;break;
			case state_t::doubt:std::cout<<doubt;break;
			case state_t::label:std::cout<<label;break;
		}
	}
	std::cout<<"\033[0m"<<std::flush;
}

bool blocks::Q_button(){
	if(if_reveal){
		return false;
	}
	if(state==state_t::label){
		return false;
	}
	if_reveal=true;
	if(if_boom){
		return true;
	}
	state=state_t::normal;
	return false;
}

class Game{
private:
    int len_x,len_y,boom_count;
    std::vector<std::vector<blocks>>map;
    std::mt19937 engine;
    std::uniform_int_distribution<int> boom_x_range;
    std::uniform_int_distribution<int> boom_y_range;
    bool start();
    bool is_game_start;
    int focus_x, focus_y; 
    void print_map();
    void Continuously_reveal(int,int);
    bool check_win();
    bool game_over;
    bool game_win;
    bool press_number();
    bool in_bounds(int x, int y) {
        return x >= 0 && x < len_x && y >= 0 && y < len_y;
    }
    void make_mine();
public:
    Game(int _len_x,int _len_y,int _boom,unsigned int seed);
    Game(std::ifstream&);
    Game()=default; 
    bool run();
    void save(std::ofstream&);
	bool operator==(const Game& other) const {
		return len_x == other.len_x && len_y == other.len_y && boom_count == other.boom_count;
	}
};
bool Game::press_number(){
	point around[]{
	{-1,-1},
	{-1,0},
	{-1,1},
	{0,-1},
	{0,1},
	{1,-1},
	{1,0},
	{1,1}
	};
	int nx,ny,label_count{};
	for(point pt:around){
		nx=focus_x+pt.x;
		ny=focus_y+pt.y;
		if(in_bounds(nx,ny) && !map[ny][nx].is_reveal() && map[ny][nx].get_state()==state_t::label){
			++label_count;
		}
	}
	if(label_count==map[focus_y][focus_x].get_value()){
		for(auto pt:around){
			int nx=focus_x+pt.x;
			int ny=focus_y+pt.y;
			if(in_bounds(nx,ny) && !map[ny][nx].is_reveal()){
        		if(map[ny][nx].Q_button()){
           			game_over=true;
       			}else{
       				if(map[ny][nx].get_state()!=state_t::label && map[ny][nx].get_value()==0)
            		Continuously_reveal(nx,ny);
        		}
    		}
		}
	}
	return false;
}
void Game::Continuously_reveal(int x,int y){
	point around[8]={
		{0,1},
		{1,0},
		{0,-1},
		{-1,0},
		{1,1},
		{-1,1},
		{1,-1},
		{-1,-1}
	};
	std::vector<point>queue;
	int index=0;
	queue.push_back(point{x,y});
	while(index< static_cast<int>(queue.size()) ){
		point current = queue[index];
		for(auto &around_point:around){
			int nx = current.x + around_point.x;
			int ny = current.y + around_point.y;
			if(in_bounds(nx, ny)&&map[ny][nx].try_reveal()){
				queue.push_back(point{nx, ny});
			}
		}
		++index;
	}
}
void Game::make_mine(){
	for(int i=0;i<boom_count;i++){
		int x, y;
		int attempts = 0;
		const int MAX_ATTEMPTS = 100;
		do {
			x = boom_x_range(engine);
			y = boom_y_range(engine);
			attempts++;
			bool too_close = false;
			for(int dx=-1;dx<=1;dx++){
				for(int dy=-1;dy<=1;dy++){
					if(in_bounds(focus_x+dx,focus_y+dy)&&focus_x+dx==x && focus_y+dy==y){
						too_close = true;
						goto FLAG;
					}
				}
			}
			FLAG: 
			if(too_close && attempts < MAX_ATTEMPTS) {
				continue;
			}
			if(map[y][x].is_boom()){
				if(attempts < MAX_ATTEMPTS) {
					continue;
				}
			}
			break;
		} while(true);
		map[y][x].set_boom();
		for(int dx=-1;dx<=1;dx++){
			for(int dy=-1;dy<=1;dy++){
				int nx = x+dx;
				int ny = y+dy;
				if(in_bounds(nx, ny) && !(dx==0 && dy==0)){
					map[ny][nx].add_value();
				}
			}
		}
	}
}
bool Game::start(){
	int ch;
	std::cout << "Starting Minesweeper Game...\n";
	std::cout << "Map size: "<<len_x<<'x'<<len_y<<", Mines: "<<boom_count;
	conin.getch();
	std::cout<<"\033[1J\033[H"<<std::endl;
	print_map();
	while((ch=conin.getch()) != 0){
		switch(ch){
			case 27:{ // ESC
				return false;
			}
			case 'w':{
				map[focus_y][focus_x].loss_focus();
				if(focus_y>0){
					focus_y--;
				}s;
				break;
			}
			case 'a':{
				map[focus_y][focus_x].loss_focus();
				if(focus_x>0){
					focus_x--;
				}
				map[focus_y][focus_x].set_focus();
				break;
			}
			case 'd':{
				map[focus_y][focus_x].loss_focus();
				if(focus_x<len_x-1){
					focus_x++;
				}
				map[focus_y][focus_x].set_focus();
				break;
			}
			case 'e':{
				map[focus_y][focus_x].E_button();
				break;
			}
			case 'q':{
				make_mine();
				is_game_start=true;
				map[focus_y][focus_x].Q_button();
				Continuously_reveal(focus_x,focus_y);
				print_map();
				return true;
			}
			default:
				break;
		}
		print_map();
	}
	return false;
}

Game::Game(int _len_x,int _len_y,int _boom,unsigned int seed):
	len_x(_len_x),len_y(_len_y),boom_count(_boom),
	is_game_start(false),game_over(false),game_win(false),
	engine(std::mt19937(seed)),
	boom_x_range(std::uniform_int_distribution<int>(0,len_x-1)),
	boom_y_range(std::uniform_int_distribution<int>(0,len_y-1)),
	focus_x(_len_x/2),focus_y(_len_y/2),
	map(std::vector<std::vector<blocks>>(len_y,std::vector<blocks>(len_x,blocks()))){
	if(len_y>0 && len_x>0 && focus_y < len_y && focus_x < len_x){
		map[focus_y][focus_x].set_focus();
	}
}

void Game::print_map(){
	std::cout<<"\033[H"; 
	std::cout << "Minesweeper Game - " << boom_count << " mines\n";
	std::cout << "====================="<<std::endl;
	
	for(int y = 0; y < len_y; y++){
		for(int x = 0; x < len_x; x++){
			map[y][x].print();
		}
		std::cout <<std::endl;
	}
	std::cout << "\nControls: WASD-move, Q-reveal, E-cycle mark, ESC-quit\n";
	std::cout << "Focus at: (" << focus_x << ", " << focus_y << ")"<<std::endl;
}

bool Game::run(){
	std::cout<<"\033[1J\033[H"<<std::flush;
	if(!is_game_start){
		if(start()){
			is_game_start=true;
		}else{
			return false;
		}
	}
	int ch;
	while((ch=conin.getch()) != 0){
		switch(ch){
			case 27:{ // ESC
				return false;
			}
			case 'w':{
				map[focus_y][focus_x].loss_focus();
				if(focus_y>0){
					focus_y--;
				}
				map[focus_y][focus_x].set_focus();
				break;
			}
			case 's':{
				map[focus_y][focus_x].loss_focus();
				if(focus_y<len_y-1){
					focus_y++;
				}
				map[focus_y][focus_x].set_focus();
				break;
			}
			case 'a':{
				map[focus_y][focus_x].loss_focus();
				if(focus_x>0){
					focus_x--;
				}
				map[focus_y][focus_x].set_focus();
				break;
			}
			case 'd':{
				map[focus_y][focus_x].loss_focus();
				if(focus_x<len_x-1){
					focus_x++;
				}
				map[focus_y][focus_x].set_focus();
				break;
			}
			case 'e':{
				map[focus_y][focus_x].E_button();
				break;
			}
			case 'q':{
  				if(map[focus_y][focus_x].is_reveal() || map[focus_y][focus_x].get_state() == state_t::label){
  					if(map[focus_y][focus_x].is_reveal())press_number();
        			break;
  				}
    			if(map[focus_y][focus_x].Q_button()){
        			game_over = true;
    			}else if(map[focus_y][focus_x].get_value() == 0){
           		 	Continuously_reveal(focus_x,focus_y);
        		}
    			break;
			}
			default:
				break;
		}
		print_map();
		if(check_win()){
            game_win = true;
           	std::cout<<"\nYou Win!\n"<<std::endl;
			conin.getch();
           	return true;
        }
		if(game_over){
			std::cout<<"\nGame Over! You hit a mine!\n"<<std::endl;
			conin.getch();
			return true;
		}
	}
	return false;
}
bool Game::check_win(){
	for(int y = 0; y < len_y; y++){
		for(int x = 0; x < len_x; x++){
			auto& block = map[y][x];
			if(!block.is_boom() && !block.is_reveal()){
				return false;
			}
		}
	}
	return true;
}
void Game::save(std::ofstream& ofs){
	int putout=boom_count|(len_y<<8)|(len_x<<13)|(static_cast<int>(game_over)<<18)|(static_cast<int>(game_win)<<19)|(static_cast<int>(is_game_start)<<20);
	ofs.write(reinterpret_cast<char*>(&putout),sizeof(int));
	for(auto &line:map){
		for(auto &block:line){
			block.save(ofs);
		}
	}
	ofs.put('\n');
	ofs.flush();
}
Game::Game(std::ifstream& ifs) {
	int getin;
	ifs.read(reinterpret_cast<char*>(&getin),sizeof(int));
	boom_count=getin&0xFF;
	len_x=(getin>>13)&0x1F;
	len_y=(getin>>8)&0x1F;
	game_over=getin&(1<<18);
	game_win=getin&(1<<19);
	is_game_start=getin&(1<<20);
    unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine = std::mt19937(seed);
    boom_x_range = std::uniform_int_distribution<int>(0, len_x - 1);
    boom_y_range = std::uniform_int_distribution<int>(0, len_y - 1);
    focus_x = len_x / 2;
    focus_y = len_y / 2;
    map = std::vector<std::vector<blocks>>(len_y, std::vector<blocks>(len_x));
    for (auto &line : map) {
        for (auto &block : line) {
            block.read(ifs);
        }
    }
	if(ifs.get()!='\n') {
		ifs.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
	}
    if (len_y > 0 && len_x > 0 && focus_y < len_y && focus_x < len_x) {
        map[focus_y][focus_x].set_focus();
    }
}
class Save_Manager{
private:
	struct game_info{
		Game game;
		std::string game_name;
	};	
	std::vector<game_info>save_list;
	std::vector<game_info>::iterator index;
	bool OK_NO;
	void print_list();
	void save_write();
	static constexpr int top_len=32;
	static constexpr int edge_len=3;
	static constexpr int name_max_len=top_len-2*edge_len;
	static const std::string bourd_top;
	static const std::string bourd_edge;
	void init();
	void play();
public:
	Save_Manager();
	void run();
	// void test(){
	// 	save_list={{Game(14,14,12,2),"game1"},{Game(14,14,12,2),"game2"},{Game(14,14,12,2),"game3"}};
	// 	index=save_list.begin();
	// 	OK_NO=true;
	// }
	void add_game(Game game,std::string name);
};

std::string const Save_Manager::bourd_edge=std::string(Save_Manager::edge_len,'*');
std::string const Save_Manager::bourd_top=std::string(Save_Manager::top_len,'*');

Save_Manager::Save_Manager():save_list{}{}

void Save_Manager::print_list(){
	std::cout<<"\033[H";
	std::cout<<bourd_top<<std::endl;
	for(auto it=save_list.begin();it!=save_list.end();++it){
		auto name=it->game_name;
		if(name.size()>name_max_len){
			name=name.substr(0,name_max_len);
		}else{
			name+=std::string(name_max_len-name.size(),' ');
		}
		if(it==index){
			name=chosen+name+unchosen;
		}
		name=bourd_edge+name+bourd_edge;
		std::cout<<name<<std::endl;
	}
	std::cout<<bourd_top<<std::endl;
	std::string _OK {"play"};
	std::string _NO {"delete"};
	if(OK_NO){
		_OK=chosen+_OK+unchosen;
	}else{
		_NO=chosen+_NO+unchosen;
	}
	std::cout<<std::string(edge_len,' ')<<_OK<<std::string(name_max_len-10,' ')<<_NO<<std::endl;
}

static const std::string FILE_HEADER = "MINESWEEPER_SAVE_V1";

void Save_Manager::init(){
    save_list.clear();
    game_info info;
    std::ifstream ifs("Minedata.ini", std::ios::in|std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open save file.");
    }
    std::string header;
    std::getline(ifs, header);
    if (header != FILE_HEADER) {
		index = save_list.begin();
        OK_NO = true;
        ifs.close();
        return;
    }
    while (std::getline(ifs, info.game_name)) {
		try{
			info.game = Game(ifs);
        	save_list.push_back(info);
		}catch(std::exception&e){
			std::cerr<<"game "<<info.game_name<<" is destroyed."<<std::flush;
			conin.getch();
			putchar('\n');
		}
		if(ifs.fail()&&!ifs.eof()&&!ifs.bad()){
			ifs.clear();
		}
    }
    ifs.close();
    index = save_list.begin();
    OK_NO = true;
}

void Save_Manager::save_write(){
    std::ofstream ofs("Minedata.ini", std::ios::out|std::ios::binary);
    if (!ofs.good()) {
        throw std::runtime_error("Unknown error when open the file.");
    }

    ofs << FILE_HEADER << '\n';
    for (auto& info : save_list) {
        ofs << info.game_name << '\n';
		try{
			info.game.save(ofs);
		}catch(std::exception&e){
			std::cerr<<e.what()<<std::endl;
			conin.getch();
			ofs.close();
			return;
		}
    }
    ofs.close();
}

void Save_Manager::play(){
	if(index->game.run()){
		index=save_list.erase(index);
	}else{
		if(!ask_YN("do you wan't to save the game?")){
			index=save_list.erase(index);
		}
	}
	std::cout<<"\033[1J\033[H"<<std::flush;
}
void Save_Manager::run(){
	std::cout<<"\033[1J\033[H"<<std::flush;
	try{
		init();
	}catch(std::exception&e){
		std::cerr<<e.what()<<std::endl<<"Press any ker to continue."<<std::flush;
		conin.getch();
		return;
	}
	print_list();
	int ch;
	while((ch=conin.getch())!=27){
		switch(ch){
		case 'w':
			if(index!=save_list.begin()){
				--index;
			}
			break;
		case 's':
			if(index==save_list.end()){
				break;
			}
			if(++index==save_list.end()){
				--index;
			}
			break;
		case 'a':
		case 'd':
			OK_NO=!OK_NO;
			break;
		case ' ':
		std::cout<<"\033[1J\033[H"<<std::flush;
			if(OK_NO){
				if(!save_list.empty())
				play();
			}else{
				if(!save_list.empty())
				index=save_list.erase(index);
			}
			if(index==save_list.end()&&!save_list.empty()) index=save_list.end()-1;
		}
		print_list();
	}
	try{
		save_write();
	}catch(std::exception&e){		
		std::cerr<<e.what()<<std::endl<<"Press any ker to continue."<<std::flush;
		conin.getch();
	}
	std::cout<<"\033[1J\033[H"<<std::flush;
}
void Save_Manager::add_game(Game game,std::string name){
	try{
		init();
	}catch(std::exception&e){
		save_list.clear();
	}
	save_list.push_back(game_info{game,name});
	try{
		save_write();
	}
	catch(std::exception&e){
		std::cerr<<e.what()<<std::endl<<"Press any key to continue."<<std::flush;
		conin.getch();
		save_list.clear();
	}
}
class Main_menu;
class New_Game_Manager{
private:
	static const std::string repared_game_str[4];
	static const Game no_return_game;
	Game game;
	int choose_index;
	void print_list();
	Game customize_game();
	void play_game();
	Main_menu *observer;//observer->Archive.add_game(game,name);
public:
	New_Game_Manager(Main_menu *_observer):choose_index(0),observer(_observer){}
	void run();
};
const std::string New_Game_Manager::repared_game_str[4]={"9x9-10 mines  ","16x16-40 mines","16x30-99 mines","Customize     "};
const Game New_Game_Manager::no_return_game=Game(1,1,1,std::chrono::system_clock::now().time_since_epoch().count());
void New_Game_Manager::print_list(){
	std::cout<<"\033[H**********************"<<std::endl;
	for(int i=0;i<4;++i){
		std::string str=repared_game_str[i];
		if(i==choose_index){
			str=chosen+str+unchosen;
		}
		str=std::string("****")+str+std::string("****");
		std::cout<<str<<std::endl;
	}
	std::cout<<"**********************"<<std::endl;
}
Game New_Game_Manager::customize_game(){
	std::cout<<"\033[1J\033[H";
	int len_x_len_y_boom_count[3]{16,16,40};
	int *ptr=len_x_len_y_boom_count;
	char ch;
	std::cout	<<"Height:"<<(ptr==len_x_len_y_boom_count?chosen:unchosen)<<len_x_len_y_boom_count[0]<<unchosen
				<<" Width:"<<(ptr==len_x_len_y_boom_count+1?chosen:unchosen)<<len_x_len_y_boom_count[1]<<unchosen
				<<" Mines:"<<(ptr==len_x_len_y_boom_count+2?chosen:unchosen)<<len_x_len_y_boom_count[2]<<unchosen
				<<std::endl;
	while((ch=conin.getch())!=27){
		switch(ch){
		case 'a':
			if(ptr!=len_x_len_y_boom_count){
				--ptr;
			}
			break;
		case 'd':
			if(ptr!=len_x_len_y_boom_count+2){
				++ptr;
			}
			break;
		case 'w':
			++*ptr;
			break;
		case 's':
			if(*ptr>1){
				--*ptr;
			}
			break;
		case ' ':
			return Game(len_x_len_y_boom_count[0],len_x_len_y_boom_count[1],len_x_len_y_boom_count[2],std::chrono::system_clock::now().time_since_epoch().count());
		}
		if(len_x_len_y_boom_count[0]>31) len_x_len_y_boom_count[0]=31;
		if(len_x_len_y_boom_count[1]>31) len_x_len_y_boom_count[1]=31; 
		if(len_x_len_y_boom_count[0]*len_x_len_y_boom_count[1]*0.25<len_x_len_y_boom_count[2]){
			len_x_len_y_boom_count[2]=len_x_len_y_boom_count[0]*len_x_len_y_boom_count[1]*0.25;
		}
		std::cout<<"\033[2J\033[H";
		std::cout	<<"Height:"<<(ptr==len_x_len_y_boom_count?chosen:unchosen)<<len_x_len_y_boom_count[0]<<unchosen
					<<" Width:"<<(ptr==len_x_len_y_boom_count+1?chosen:unchosen)<<len_x_len_y_boom_count[1]<<unchosen
					<<" Mines:"<<(ptr==len_x_len_y_boom_count+2?chosen:unchosen)<<len_x_len_y_boom_count[2]<<unchosen
					<<std::endl;
	}
	return no_return_game;
}

void New_Game_Manager::run(){
	std::cout<<"\033[1J\033[H"<<std::flush;
	print_list();
	int ch;
	while((ch=conin.getch())!=27){
		switch(ch){
		case 's':
			if(choose_index!=3){
				choose_index++;
			}
			break;
		case 'w':
			if(choose_index!=0){
				choose_index--;
			}
			break;
		case ' ':
			switch(choose_index){
			case 0:
				game=Game(9,9,10,std::chrono::system_clock::now().time_since_epoch().count());
				break;
			case 1:
				game=Game(16,16,40,std::chrono::system_clock::now().time_since_epoch().count());
				break;
			case 2:
				game=Game(30,16,99,std::chrono::system_clock::now().time_since_epoch().count());
				break;
			case 3:
				game=customize_game();
				break;
			}
			play_game();
			std::cout<<"\033[1J\033[H"<<std::flush;
			return;
		}
		print_list();
	}
}

class Main_menu{
	friend class New_Game_Manager;
	Save_Manager Archive;
	New_Game_Manager start_new;
	int choose_index;//1-new game,2-select Archive,3-exit
	void select_Archive(){
		Archive.run();
	}
	void new_game(){
		start_new.run();
	}
	static const std::string string_list[3];
	void print();

public:
	Main_menu();
	void run();
};
void New_Game_Manager::play_game(){
	if(game==no_return_game)return;
	if(!game.run()&&ask_YN("do you wan't to save the game?")){
		std::string name;
		std::cout<<"\nPlease enter the name of the game:"<<std::flush;
		std::lock_guard<Conioer> guard(conin);
		std::getline(std::cin,name);
		observer->Archive.add_game(game,name);
	}
}
Main_menu::Main_menu():choose_index(1),Archive(),start_new(this){}
const std::string Main_menu::string_list[3]={"New Game","Select Archive","Exit"};
void Main_menu::print(){
	std::cout<<"\033[H";
	std::cout<<"**********************************************************"<<std::endl;
	std::cout<<"****@@@*******@@@****@@@@@@***@@@*******@@***@@@@@@@@@@***"<<std::endl;
	std::cout<<"***@@*@@*****@@*@@*****@@*****@@*@@*****@@***@@***********"<<std::endl;
	std::cout<<"***@@**@@***@@**@@*****@@*****@@**@@****@@***@@***********"<<std::endl;
	std::cout<<"***@@***@@*@@***@@*****@@*****@@***@@***@@***@@@@@@@@*****"<<std::endl;
	std::cout<<"***@@****@@@****@@*****@@*****@@****@@**@@***@@***********"<<std::endl;
	std::cout<<"***@@***********@@*****@@*****@@*****@@*@@***@@***********"<<std::endl;
	std::cout<<"***@@***********@@***@@@@@@***@@*******@@@***@@@@@@@@@@***"<<std::endl;
	std::cout<<"**********************************************************"<<std::endl;
	std::cout<<"\n\n"<<std::endl;
	std::cout<<"**********************************************************"<<std::endl;
	for(int i=1;i<=3;i++){
		std::string str=string_list[i-1];
		str+=std::string(50-str.size(),' ');
		if(i==choose_index){
			str=chosen+str+unchosen;
		}
		str=std::string("****")+str+std::string("****");
		std::cout<<str<<std::endl;
	}
	std::cout<<"**********************************************************"<<std::endl;
}
void Main_menu::run(){
	std::cout<<"\033[1J\033[H"<<std::flush;
	print();
	int ch;
	while((ch=conin.getch())!=27){
		switch(ch){
		case 's':
			if(choose_index!=3){
				choose_index++;
			}
			break;
		case 'w':
			if(choose_index!=1){
				choose_index--;
			}
			break;
		case ' ':
			switch(choose_index){
			case 1:
				new_game();
				break;
			case 2:
				select_Archive();
				break;
			case 3:
				return;
			}
		}
		print();
	}
}
int main(){
	Main_menu menu;
	menu.run();
    return 0;
}
