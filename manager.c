#include <stdlib.h>
#include <ncurses.h>
#include <menu.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

//Global variables
int show_hidden = 1;
int clear_command_timer = 2;    //in seconds
WINDOW *finder, *path_win, *command_win, *info_win;

int set_parent(char *path);
void clear_command (int signum);
int ex_command(char *command);
void write_path_win(char *string);
void help_page();
void print_help(char *command);

int main(int argc, char *argv[]){
    int loop = 1, c;
    int w, h;

    int path_size = 20;
    char *path = calloc(path_size, sizeof(char));
    char *next_path = calloc(path_size, sizeof(char));
    strcpy(path, "/");
    DIR *cur_dir;
    struct stat *cur_stat, *sel_stat = malloc(sizeof(struct stat));
    struct dirent *dir_content = malloc(sizeof(struct dirent));
    int dir_size = 20, n_in_dir;
    char **sub = calloc(dir_size, sizeof(char *));
    char *selected;
    char *command = calloc(10, sizeof(char));

    ITEM **dir_row;
    MENU *list;

    signal (SIGALRM, clear_command);

    initscr();
    getmaxyx(stdscr, h, w);
	keypad(stdscr, TRUE);

    path_win = newwin(1, COLS, 0, 0);
    finder = newwin(LINES-3, COLS/2, 1, 0);
    info_win = newwin(LINES-3, COLS/2, 1, (COLS/2)+1);
    command_win = newwin(3, COLS, LINES-2, 0);
    keypad(finder, TRUE);

    while(loop){
        cbreak();
        noecho();
        curs_set(0);

        write_path_win(path);

        box(finder, '|', '-');

        cur_dir = opendir(path);
        stat(path, cur_stat);
        if(cur_dir){
            //Allocate space and create sub-dir/files
            for(n_in_dir = 0; (dir_content = readdir(cur_dir)); n_in_dir++){
                if(n_in_dir >= dir_size){
                    dir_size *= 2;
                    sub = realloc(sub, dir_size*sizeof(char *));
                }
                sub[n_in_dir] = calloc(256, sizeof(char));
                strcpy(sub[n_in_dir], dir_content->d_name);

            }
            // for(int i = 0; i < n_in_dir; i++)        //DEBUGGING: print dir content on stderr
            //     fprintf(stderr, "%s\n", sub[i]);
            if(!show_hidden){
                int hidden = 0;
                for(int i = 0; i < n_in_dir; i++){
                    if(!strcmp(sub[i], "."))
                        ;
                    else if(!strcmp(sub[i], ".."))
                        ;
                    else if(sub[i][0] == '.')
                        hidden++;
                }
                dir_row = (ITEM **)calloc(n_in_dir - hidden + 1, sizeof(ITEM *));
                dir_row[0] = new_item(sub[0], "");  //.
                dir_row[1] = new_item(sub[1], "");  //..
                for(int i = 2, j = hidden+2; j < n_in_dir; i++, j++){
                    dir_row[i] = new_item(sub[j], "");
                }
            }else{
                dir_row = (ITEM **)calloc(n_in_dir + 1, sizeof(ITEM *));
                for(int i = 0; i < n_in_dir; i++){
                    dir_row[i] = new_item(sub[i], "");
                }
            }
            dir_row[n_in_dir] = (ITEM *)NULL;

            list = new_menu(dir_row);
            set_menu_win(list, finder);
            set_menu_mark(list, " * ");
            set_menu_format(list, LINES-4, 1);
            wmove(finder, 2, 0);
            post_menu(list);
            wrefresh(finder);
            while((c = wgetch(finder)) != 'q'){

                if(c == 10)
                    break;
                if(c == 'c')
                    break;
                if(c == 8)
                    break;
                switch (c) {
                    case KEY_UP:
                    case 'u':
                        menu_driver(list, REQ_UP_ITEM);
                    break;
                    case KEY_DOWN:
                    case 'd':
                        menu_driver(list, REQ_DOWN_ITEM);
                    break;
                    case 'c':

                    break;
                }
                wrefresh(finder);
                werase(info_win);
                wrefresh(info_win);

                //Selecting file and preparing next path
                selected = (char *)item_name(current_item(list));
                if(strcmp(selected, ".") != 0){
                    if(strcmp(selected, "..") != 0){
                        if((strlen(path) + strlen(selected)) >= path_size-2){
                            path_size *= 2;
                            path = realloc(path, path_size*sizeof(char));
                            next_path = realloc(next_path, path_size*sizeof(char));
                        }

                        strcpy(next_path, path);
                        strcat(next_path, selected);
                        strcat(next_path, "/");
                        fprintf(stderr, "selected: %s\n", selected);
                        fprintf(stderr, "path: %s\n", path);
                        fprintf(stderr, "next_path: %s\n", next_path);

                        stat(next_path, sel_stat);
                        mvwprintw(info_win, 2, 2, "Inode number: %d", sel_stat->st_ino);
                        mvwprintw(info_win, 4, 2, "File size: %d B", sel_stat->st_size);
                        mvwprintw(info_win, 6, 2, "Last access: %d s since epoch", sel_stat->st_atimespec.tv_sec);
                        mvwprintw(info_win, 8, 2, "Last modification: %d s since epoch", sel_stat->st_mtimespec.tv_sec);
                        wrefresh(info_win);

                    }else{
                        werase(info_win);
                        wrefresh(info_win);
                    }
                }else{
                    werase(info_win);
                    wrefresh(info_win);
                }

            }
            if(c == 10){
                if(strcmp(selected, ".") != 0){
                    if(strcmp(selected, "..") != 0){
                        strcpy(path, next_path);
                    }else{
                        set_parent(path);
                    }
                }
            }
            if(c == 8){
                set_parent(path);
            }
            if(c == 'c'){
                alarm(0);
                werase(command_win);
                wprintw(command_win, "Command: ");
                curs_set(1);
                echo();
                wscanw(command_win, "%[a-z0-9 ]", command);
                fprintf(stderr, "Command: %s\n", command);
                werase(command_win);
                wrefresh(command_win);
                if(ex_command(command)){
                    wprintw(command_win, "Command not found");
                    alarm(clear_command_timer);
                }
                wrefresh(command_win);
            }

            if(c == 'q')
                loop = 0;

            unpost_menu(list);
        }else{
            werase(command_win);
            wprintw(command_win, "%s is not a directory", selected);
            wrefresh(command_win);
            alarm(clear_command_timer);
            set_parent(path);
        }
    }
    //Free space for contents
    for(int i = 0; i < n_in_dir; i++){
        free(sub[i]);
        free_item(dir_row[i]);
    }
    free_menu(list);

    delwin(finder);
    endwin();
    return 0;
}

//Use set_parent to set parent dir in path
int set_parent(char *path){
    int len = strlen(path);
    if(len == 1)
        return 0;
    for(int i = len-2; i >= 0; i--){
        if(path[i] == '/'){
            path[i+1] = '\0';
            return 0;
        }
    }
    return 1;
}

//Parsing and execution of commands
int ex_command(char *command){
    char **comm = calloc(1, sizeof(char*));
    char *tmp;

    tmp = strtok(command, " ");
    if(!tmp)
        return -1;
    comm[0] = calloc(strlen(tmp), sizeof(char));
    strcpy(comm[0], tmp);
    int i = 0;
    while((tmp = strtok(NULL, " "))){
        i++;
        comm = realloc(comm, i*sizeof(char*));
        comm[i] = calloc(strlen(tmp), sizeof(char));
        strcpy(comm[i], tmp);
    }

    //Command selection
    if(!strcmp(comm[0], "show")){
        if(comm[1]){
            if(!strcmp(comm[1], "hidden")){
                show_hidden = 1;
                return 0;
            }
        }
    }else if(!strcmp(comm[0], "hide")){
        if(comm[1]){
            if(!strcmp(comm[1], "hidden")){
                show_hidden = 0;
                return 0;
            }
        }
    }else if(!strcmp(comm[0], "help")){
        help_page();
        return 0;
    }

    return -1;
}

//Open help page
void help_page(){
    char *selected;
    int c;

    write_path_win("HELP [press q to exit this page]");
    werase(finder);

    ITEM **com_row;
    MENU *list;
    com_row = (ITEM **)calloc(3 + 1, sizeof(ITEM *));
    com_row[0] = new_item("show", "");
    com_row[1] = new_item("hide", "");
    com_row[2] = new_item("help", "");

    com_row[3] = (ITEM *)NULL;

    list = new_menu(com_row);
    set_menu_win(list, finder);
    set_menu_mark(list, " * ");
    set_menu_format(list, LINES-4, 1);
    wmove(finder, 2, 0);
    post_menu(list);
    wrefresh(finder);

    while((c = wgetch(finder)) != 'q'){
        switch (c) {
            case KEY_UP:
            case 'u':
                menu_driver(list, REQ_UP_ITEM);
            break;
            case KEY_DOWN:
            case 'd':
                menu_driver(list, REQ_DOWN_ITEM);
            break;
        }
        selected = (char *)item_name(current_item(list));
        mvwprintw(info_win, 0, 2, "%s", selected);
        print_help(selected);
        wrefresh(info_win);
        wrefresh(finder);
    }
    werase(info_win);
    wrefresh(info_win);

    unpost_menu(list);
}

void print_help(char *command){
    werase(info_win);
    if(!strcmp(command, "show")){
        mvwprintw(info_win, 2, 2, "Syntax: show <arg>");
        mvwprintw(info_win, 3, 2, "Is used to set the visibility of the argument.");
        mvwprintw(info_win, 4, 2, "Possible args:\n\thidden: hidden files\n\t");
    }
    if(!strcmp(command, "hide")){
        mvwprintw(info_win, 2, 2, "Syntax: hide <arg>");
        mvwprintw(info_win, 3, 2, "Is used to unset the visibility of the argument.");
        mvwprintw(info_win, 4, 2, "Possible args:\n\thidden: hidden files\n\t");
    }
    if(!strcmp(command, "help")){
        mvwprintw(info_win, 2, 2, "help command shows this help page.");
        mvwprintw(info_win, 3, 2, "Nothing else.");
    }
}

//Write string to path_win and refresh
void write_path_win(char *string){
    werase(path_win);
    mvwprintw(path_win, 0, 0, "%s", string);
    wrefresh(path_win);
}

void clear_command (int signum){
    werase(command_win);
    wrefresh(command_win);
}
