#include "splashkit.h"
#include <format>
#include <cstdlib>
#include <stdio.h>
#include <new> 

using std::to_string;

//This part (The dynamic array) is a exactly the same code as in the Deep Dive Memory Task, 
//But I might make slight adjustments to it throughout the project to suit my needs.
template <typename T>
struct dynamic_array
{
    int capacity;
    int size;
    T *data;

    dynamic_array(int capacity)
    {
        size = 0;
        data = new T[capacity];

        for (int i = 0; i < capacity; i++)
        {
            new(&this->data[i]) T();
        }
        if (data == nullptr)
        {
            this->capacity = 0;
        }
        else
        {
            this->capacity = capacity;
        }
    }
    ~dynamic_array()
    {
        // Clear to ensure we remove any data from memory before freeing it
        for (int i = 0; i < capacity; i++)
        {
            data[i].~T();
        }
        size = 0;
        capacity = 0;

        // Free the data in the array
        delete[] data;
        // Free the array itself
        
    }
    bool resize(int new_capacity)
    {

        // Call destructors if we are reducing size
        for(int i = capacity - 1; i >= (int)new_capacity; i--)
        {
            data[i].~T();
        }

        // Allocate a new array with the new capacity and store it in a local variable to check it is not null
        
        T *new_data = (T *)realloc(data, new_capacity * sizeof(T));


        if (new_data == nullptr)
        {
            printf("Memory allocation failed\n");
            return false; // Memory allocation failed
        }
        // Call constructors if we increased size
        for(int i = capacity; i < new_capacity; i++)
        {
            new(&new_data[i]) T();
        }

        // Update the array's data and capacity
        data = new_data;
        capacity = new_capacity;

        // Update the size if the new capacity is smaller than the current size
        if (new_capacity < size)
        {
            size = new_capacity;
        }

        return true; // Resizing succeeded
    }

    bool add(T value)
    {
        // Check if we need to resize the array
        if (size >= capacity)
        {
            // Double the capacity of the array
            if (!resize(capacity * 2 + 1))
            {
                printf("Memory allocation failed\n");
                return false; // Memory allocation failed
            }
        }

        // Add the new value to the end of the array and increment the size
        data[size] = value;
        size++;

        return true; // Adding succeeded
    }
    const T &operator[](unsigned int index) const
    {
        // Check if the index is out of bounds
        if (index >= size)
        {
            // The index is out of bounds, so return the default value
            return data[0];
        }

        return data[index];
    }
    T &operator[](unsigned int index) 
    {
        // Check if the index is out of bounds
        if (index >= size)
        {
            // The index is out of bounds, so return the default value
            return data[0];
        }

        return data[index];
    }

    T get(unsigned int index)
    {
        // Check if the index is out of bounds
        if (index < 0 || index >= size)
        {
            return 0; // Return the default value
        }
        return data[index]; // Return the value at the index
    }
    bool set(unsigned int index, T value)
    {
        // Check if the index is out of bounds
        if (index >= size)
        {
            // The index is out of bounds, so do nothing
            return false;
        }

        data[index] = value;
        return true;
    }
    void print()
    {
        //Print the capacity and size of the array
        printf("Dynamic array capacity: %d\n", capacity);
        printf("Dynamic array size: %d\n", size);
        printf("Dynamic array: [");
        for (int i = 0; i < size; i++)
        {
            printf("%d", data[i]);
            if (i < size - 1)
            {
                printf(", ");
            }
        }
        printf("]\n");
    }
};
//End of dynamic array code

const int SCREEN_HEIGHT = 720;
const int SCREEN_WIDTH = 1080;

const int BUFFER = 250;
const int NUM_IMAGES = 5;

const int FONT_SIZE = 30;       
const font FONT1 = load_font("font1", "Roboto-italic.ttf");

bitmap IMAGES[NUM_IMAGES];

const long WIND_CHANGE_TIME = 4000; // Every 4 seconds the wind changes direction
const int MAX_WIND = 5; // Maximum wind speed

//Add all the rock related functions to this struct
struct rock_{
    double x_pos;
    double y_pos;
    bitmap *image;
    double velocity[2];
    bool draw;
    bool missed;
    bool hit;

    rock_()
    {  
        int rock_i = rnd(NUM_IMAGES);
        image = &IMAGES[rock_i];

        double x = rnd(-bitmap_width(*image)/2 + bitmap_width(*image)/15, SCREEN_WIDTH - bitmap_width(*image)/2 - bitmap_width(*image)/15)*1.0;

        x_pos = x;
        y_pos = -bitmap_height("Rock_"+to_string(rock_i))*0.45;
        velocity[0] = 0;
        velocity[1] = rnd(20,100)/100.0;
        draw=true;
        missed=false;
        hit=false;
    }
    ~rock_()
    {
    }
    void draw_rock()
    {
        draw_bitmap(*image, x_pos, y_pos, option_scale_bmp(0.1,0.1));
        // track_rock();//For debugging purposes
        x_pos+=velocity[0];
        y_pos+=velocity[1];
    }

    //This function is for debugging purposes 
    void track_rock()
    {
        double x = x_pos + bitmap_width(*image)/2;
        double y = y_pos + bitmap_height(*image)/2;
        double radius = bitmap_width(*image)/25;
        draw_circle(color_black(), x, y, radius);
    }
};

struct player_
{
    double health;
    point_2d player_pos;
    double radius;

    player_(int _health)
    {
        health = _health;
        player_pos.x = SCREEN_WIDTH/2;
        player_pos.y = SCREEN_HEIGHT - 50;
        radius = 50;
    }
    ~player_()
    {
    }
    
};

struct stats_page
{
    int score;
    int dodge_accuracy;
    dynamic_array<rock_ *> *rock_history;
    stats_page(int _score, dynamic_array<rock_ *> *_rock_history)
    {
        score = _score;
        rock_history = _rock_history;
    }
    ~stats_page()
    {
    }

    void calc_stats()
    {
        double missed = 0;
        double hit = 0;
        for (int i = 0; i < rock_history->size; i++)
        {
            rock_ *rock = rock_history->data[i];
            if (rock->missed)
            {
                missed++;
                // write_line("Missed Rock: " + to_string(i));
            }
            else if (rock->hit)
            {
                hit++;
            }
            // write_line("Rock " + to_string(i) + " : " + to_string(rock->missed) + " : " + to_string(rock->hit));
        }
        // write_line(" Missed: " + to_string(missed) + " Hit: " + to_string(hit));

        if (hit+missed==0)
        {
            dodge_accuracy = 0;
        }
        else
        {
            dodge_accuracy = ((missed/(hit+missed))*100);
            write_line("Dodge Accuracy: " + to_string(dodge_accuracy) + "%");
        }
    }

    bool mouse_on_button(double x)
    {
        return (mouse_x() > x && mouse_x() < x+SCREEN_WIDTH/5 && mouse_y() > SCREEN_HEIGHT*5/6 && mouse_y()< SCREEN_HEIGHT*5/6 + 100);
    }

    /*
    Draw the buttons in the menu
    */
    void draw_button(double x)
    {
        color btn_color;
        if (mouse_on_button(x))
        {
            btn_color = color_dim_gray();
        }
        else
        {
            btn_color = color_dark_gray();
        }
        fill_rectangle(btn_color, x, SCREEN_HEIGHT*5/6, SCREEN_WIDTH/5,100);
    }
    int draw_stats()
    {
        calc_stats();
        while(!quit_requested())
        {
            process_events();

            clear_screen(color_white());
            draw_text("Game Over", color_black(), FONT1, FONT_SIZE*5, SCREEN_WIDTH/2 -FONT_SIZE*10 ,SCREEN_HEIGHT/3 - 120 );
            draw_text("Score: " + to_string(score), color_black(), FONT1, FONT_SIZE, SCREEN_WIDTH/2 -FONT_SIZE*10 ,SCREEN_HEIGHT/3  + FONT_SIZE*2 );
            draw_text("Dodge Accuracy: " + to_string((int)dodge_accuracy) + "%", color_black(), FONT1, FONT_SIZE, SCREEN_WIDTH/2 -FONT_SIZE*10 ,SCREEN_HEIGHT/2);
            

            

            for (int i =0; i < 2; i++)
            {
                draw_button((1+2*i)*SCREEN_WIDTH/5);

                if (mouse_on_button((1+2*i)*SCREEN_WIDTH/5) && mouse_clicked(LEFT_BUTTON))
                {
                    return i;
                }
            }
            draw_text("EXIT",color_red(),FONT1, FONT_SIZE,SCREEN_WIDTH/5 +FONT_SIZE*2, SCREEN_HEIGHT*4/6 + 150);
            draw_text("MENU",color_red(),FONT1, FONT_SIZE,SCREEN_WIDTH*3/5 +FONT_SIZE*2, SCREEN_HEIGHT*4/6 + 150);

            refresh_screen();
        }
        return 0;
    }
};

struct menu
{    
    menu()
    {
    }

    ~menu()
    {
    }
    /*
    Returns whether the mouse is on the button or not
    */
    bool mouse_on_button(double y)
    {
        return (mouse_x() > (SCREEN_WIDTH/3) && mouse_x() < 2 * (SCREEN_WIDTH/3) && mouse_y() > y && mouse_y() < y+100);
    }

    /*
    Draw the buttons in the menu
    */
    void draw_button(double y)
    {
        color btn_color;
        if (mouse_on_button(y))
        {
            btn_color = color_dim_gray();
        }
        else
        {
            btn_color = color_dark_gray();
        }
        fill_rectangle(btn_color, SCREEN_WIDTH/3, y, SCREEN_WIDTH/3,100);
    }


    /*
    The menu before start playing
    */
    int draw_menu()
    {
        while(!quit_requested())
        {
            process_events();

            clear_screen(color_white());
            draw_text("......ROCK DODGER......", color_orange(), FONT1, FONT_SIZE*2, SCREEN_WIDTH/2 -FONT_SIZE*10 ,SCREEN_HEIGHT/3 - 120 );

            for (int i =0; i < 400; i+=120)
            {
                draw_button(SCREEN_HEIGHT/3 + i);

                if (mouse_on_button(SCREEN_HEIGHT/3 + i) && mouse_clicked(LEFT_BUTTON))
                {
                    return i/120;
                }
            }
            draw_text("EXIT",color_white(),FONT1, FONT_SIZE,SCREEN_WIDTH/2 -FONT_SIZE*2, SCREEN_HEIGHT/3 + 20);
            draw_text("EASY", color_white(),FONT1, FONT_SIZE, SCREEN_WIDTH/2  -FONT_SIZE*2, SCREEN_HEIGHT/3 + 140);
            draw_text("MEDIUM", color_white(),FONT1, FONT_SIZE, SCREEN_WIDTH/2  -FONT_SIZE*2, SCREEN_HEIGHT/3 + 260);
            draw_text("HARD", color_white(),FONT1, FONT_SIZE, SCREEN_WIDTH/2 -FONT_SIZE*2, SCREEN_HEIGHT/3 + 380);
            refresh_screen();
        }
        return 0;
    }
    
};

struct game_state
{
    int wind;
    player_ *player;
    bool over;
    int score;
    timer game_clock;
    dynamic_array<rock_ *> *rock_history;
    dynamic_array<rock_ *> *rock_queue;
    int rock_release;
    long next_rock_time;
    long wind_change_time;
    int last_added_rock;
    timer wind_clock;
    double max_health;

    double rock_softness;//To make the rock hurt less
    double acceleration;//To increase falling rate

    game_state(double _dif)
    {
        // write_line("Difficulty: " + to_string(_dif));
        game_clock = create_timer("game_clock");
        wind_clock = create_timer("wind_clock");

        if (_dif == 0)
        { 
            _dif = 0.0001;
        }

        rock_softness = 2 / (_dif);//To make the rock hurt less
        acceleration = 0.025 * (_dif);
        max_health = 30.0/(_dif);
        player = (new player_(max_health));
        over = ((int)_dif == 0);
        score = 0;
        wind_change_time = WIND_CHANGE_TIME;

        rock_history = new dynamic_array<rock_ *>(0);
        rock_queue = new dynamic_array<rock_ *>(0);

        rock_release = 0;
        next_rock_time = 1000;

        load_images();
        // write_line("Loaded images");
    }
    ~game_state()
    {
        for (int i = 0; i < rock_queue->size;    i++) 
        {
            delete (rock_queue->data)[i];
        }
        
        delete player;
        delete rock_history;
        delete rock_queue;
    }

    void load_images()
    {
        for (int i=0; i<NUM_IMAGES; i++)
        {
            IMAGES[i] = load_bitmap("Rock_"+to_string(i), "./" + to_string(i) + ".png");
            // write_line("Loaded image: " + to_string(i) + ".png");
        }
    }

    void populate_rock_queue()
    {
        for (int i = rock_queue->size; i < rock_release*2 + BUFFER; i++)
        {
            // write_line(to_string(j));
            rock_ * new_rock = new rock_();
            rock_queue->add(new_rock);
        }
    }

    void remove_rock(rock_ &rock)
    {
        // write_line("Rock removed: " + to_string(rock.x_pos + bitmap_width(*rock.image)/2));
        // write_line(rock_history->size);
        rock_history->add(&rock);
        rock.draw = false;
    }

    void draw_rocks()
    {
        int j;
        for (int i = 0; i< rock_release; i++)
        {
            rock_ *rock = (*rock_queue)[i];
            if (!rock->draw)
            {
                continue;
            }
            else
            {
                // j++;//Track number of drawn rocks
                // track_rock(*rock); //Bounding boxes of rocks
                rock->draw_rock();
                rock->velocity[0] = wind*0.1;
                if (circles_intersect(
                    (rock->x_pos + (double) bitmap_width(*rock->image)/2),
                    (rock->y_pos + (double) bitmap_height(*rock->image)/2),
                    bitmap_width(*rock->image)/25,
                    player->player_pos.x,
                    player->player_pos.y,
                    player->radius)
                )
                {
                    player->health-=rock->velocity[1]/rock_softness;
                    rock->hit = true;
                    remove_rock(*rock);
                }
                
                else if (((*rock).y_pos + bitmap_height(*(*rock).image)/2)>=SCREEN_HEIGHT && !(*rock).missed)
                {
                    // printf("WORKS\n");
                    rock->missed = true;
                    // printf("WORKS\n");
                    remove_rock(*rock);
                    // write_line("Missed Rock: " + to_string(i));
                }
            } 
            // printf("%d\n", j);
        }
    }

    void handle_mechanics()
    {
        if (timer_ticks(game_clock)>next_rock_time && rock_release < rock_queue->size)
        {
            rock_release++;
            reset_timer(game_clock);
            next_rock_time = rnd(500, 1500)/(1 + (rock_release*acceleration));
        }
        if (timer_ticks(wind_clock)>wind_change_time)
        {
            reset_timer(wind_clock);
            if (rnd(-1,1)>=0)
            {   
                wind = rnd(1,MAX_WIND);
            }
            else
            {
                wind = -rnd(1,MAX_WIND);

            }
            wind_change_time = rnd(WIND_CHANGE_TIME/2, WIND_CHANGE_TIME);
        } 
        if (player->health <=0)
        {
            //STATS PAGE MENU
            over = true;
        }
    }

    void debug_statements()
    {
        write_line("Rock Release: " + to_string(rock_release));
        write_line("Rock Queue Size: " + to_string(rock_queue->size));
        write_line("Rock History Size: " + to_string(rock_history->size));
    }

    void handle_user_inputs()
    {
        if (key_down(Q_KEY))
        {
            over = true;
        }
        if (key_down(LEFT_KEY) && player->player_pos.x >= player->radius)
        {
            player->player_pos.x -= 0.5;
        }
        if (key_down(RIGHT_KEY) && player->player_pos.x <= (SCREEN_WIDTH - player->radius))
        {
            player->player_pos.x += 0.5;
        }
        if (key_down(SPACE_KEY))
        {
            debug_statements();
        }
    
    }
    void draw_health()
    {
        // draw_text("Health Remaining : " + to_string((int) player->health), color_black(), FONT1, FONT_SIZE, 20 ,20 );
        double y_start = SCREEN_HEIGHT/10;
        double x_start = 6 * SCREEN_WIDTH/10;
        double width = SCREEN_WIDTH/4;
        double height = 20;

        double health_width = width * (player->health/max_health);
        fill_rectangle(color_red(), x_start, y_start, width, height);
        fill_rectangle(color_light_green(), x_start, y_start, health_width, height);
    }

    void draw_player()
    {
        fill_circle(color_black(), player->player_pos.x,player->player_pos.y - player->radius - 10,player->radius);
    }

    void render_game()
    {
        start_timer(game_clock);
        start_timer(wind_clock);
        while (!quit_requested())
        {   
            // printf("Game started1\n");
            if (over)
            {
                return;
            }
            process_events();

            populate_rock_queue();
            // printf("Game started2\n");

            handle_user_inputs();
            // printf("Game started3\n");

            handle_mechanics();
            // printf("Game started4\n");

            clear_screen(color_white());
            // printf("Game started5\n");

            draw_rocks();
            // printf("Game started6\n");

            draw_player();
            // printf("Game started7\n");

            draw_health();
            // printf("Game started8\n");

            refresh_screen();
            // printf("Game started9\n");

        }
    }
};

int main()  
{   
    open_window("ROCK DODGER", SCREEN_WIDTH, SCREEN_HEIGHT);
    while (true)
    {
        menu *game_menu = new menu();
        game_state *game = new game_state((double)game_menu->draw_menu());
        delete game_menu;
        write_line("Game started");
        game->render_game();
        write_line("Game ended");
      
        // game->rock_history->print();
        stats_page stats = stats_page(game->score, game->rock_history);        
        int user_opt = stats.draw_stats();

        delete game;
        write_line("Game deleted");
        if (user_opt == 0)
        {
           break;
        }
    } 
    return 0;
}