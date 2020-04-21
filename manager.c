#include <stdlib.h>
#include <ncurses.h>
#include <menu.h>
#include <dirent.h>
#include <string.h>

//Global variables
int show_hidden = 1;

int set_parent(char *path);
int split(char *string, char **splitted);
int ex_command(char *command);

int main(int argc, char *argv[]){
    int loop = 1, c;
    int w, h;

    int path_size = 20;
    char *path = calloc(path_size, sizeof(char));
    strcpy(path, "/");
    DIR *cur_dir;
    struct dirent *dir_content = malloc(sizeof(struct dirent));
    int dir_size = 20, n_in_dir;
    char **sub = calloc(dir_size, sizeof(char *));
    char *selected;
    char *command = calloc(10, sizeof(char));

    WINDOW *finder, *path_win, *command_win;
    ITEM **dir_row;
    MENU *list;

    initscr();
    getmaxyx(stdscr, h, w);
	keypad(stdscr, TRUE);

    path_win = newwin(1, COLS, 0, 0);
    finder = newwin(LINES-3, COLS-20, 1, 0);
    command_win = newwin(3, COLS, LINES-2, 0);
    keypad(finder, TRUE);

    while(loop){
        cbreak();
        noecho();
        curs_set(0);

        werase(path_win);
        mvwprintw(path_win, 0, 0, "%s", path);
        wrefresh(path_win);

        box(finder, '|', '-');

        cur_dir = opendir(path);
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
            for(int i = 0; i < n_in_dir; i++)
                fprintf(stderr, "%s\n", sub[i]);
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
            }
            if(c == 10){
                selected = (char *)item_name(current_item(list));
                if(strcmp(selected, ".") != 0){
                    if(strcmp(selected, "..") != 0){
                        if((strlen(path) + strlen(selected)) == path_size-2){
                            path_size *= 2;
                            path = realloc(path, path_size*sizeof(char));
                        }
                        strcat(path, selected);
                        strcat(path, "/");
                    }else{
                        set_parent(path);
                    }
                }
            }
            if(c == 8){
                set_parent(path);
            }
            if(c == 'c'){
                werase(command_win);
                wprintw(command_win, "Command: ");
                curs_set(1);
                echo();
                wscanw(command_win, "%[a-z0-9 ]", command);
                fprintf(stderr, "Command: %s\n", command);
                if(ex_command(command)){
                    wprintw(command_win, "Comand not found");
                    wrefresh(command_win);
                }
                wrefresh(command_win);
            }

            if(c == 'q')
                loop = 0;

            unpost_menu(list);
            werase(command_win);
            wrefresh(command_win);
        }else{
            wprintw(command_win, "%s is not a directory", selected);
            wrefresh(command_win);
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
    char *comm;
    comm = strtok(command, " ");
    if(!comm)
        return -1;
    fprintf(stderr, "%s\n", comm);
    if(!strcmp(comm, "show")){
        comm = strtok(NULL, " ");
        fprintf(stderr, "%s\n", comm);
        if(!strcmp(comm, "hidden")){
            show_hidden = 1;
            return 0;
        }
    }else if(!strcmp(comm, "hide")){
        comm = strtok(NULL, " ");
        fprintf(stderr, "%s\n", comm);
        if(!strcmp(comm, "hidden")){
            show_hidden = 0;
            return 0;
        }
    }
    return -1;
}
