// File: rock.cpp
// Description: Implements the “Rock Dodger” game using SplashKit.
//   - Loads assets (bitmaps, fonts)
//   - Manages dynamic arrays of falling rocks and power‑ups
//   - Handles game state, input, rendering, collisions, and scoring

#include "splashkit.h"
#include <cstdlib>
#include <stdio.h>
#include <new> 

using std::to_string;

const int SCREEN_HEIGHT = 720;
const int SCREEN_WIDTH = 1080;

const int BUFFER = 250;
const int FONT_SIZE = 30;       
const font FONT1 = load_font("font1", "Roboto-italic.ttf");

bitmap IMAGES[7];

const long WIND_CHANGE_TIME = 4000; // Every 4 seconds the wind changes direction
const int MAX_WIND = 5; // Maximum wind speed

//Probabilities of power up drops, the rest will be rocks
const double POTION_RATE = 0.03; 
const double COIN_RATE = 0.2;
const double TIME_SLOW_RATE = 0.1;

const long MAX_TIME_SLOW = 8000; // Maximum time slow in milliseconds

// Template dynamic_array<T>
// A simple resizable array with manual memory management.
// - capacity: total allocated slots
// - size: current number of elements
// - data: raw pointer to T elements
template <typename T>
struct dynamic_array
{
    int capacity;
    int size;
    T *data;

    // Constructor: allocate raw memory and default-construct all slots
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

    // Destructor: call destructors on all elements and free memory
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

    // resize: adjust capacity up or down, preserving elements where possible
    bool resize(int new_capacity)
    {
        for(int i = capacity - 1; i >= (int)new_capacity; i--)
        {
            data[i].~T();
        }

        T *new_data = (T *)realloc(data, new_capacity * sizeof(T));

        if (new_data == nullptr)
        {
            printf("Memory allocation failed\n");
            return false; // Memory allocation failed
        }

        for(int i = capacity; i < new_capacity; i++)
        {
            new(&new_data[i]) T();
        }

        data = new_data;
        capacity = new_capacity;

        if (new_capacity < size)
        {
            size = new_capacity;
        }

        return true; // Resizing succeeded
    }

    // add: append new element, grow if needed
    bool add(T value)
    {
        if (size >= capacity)
        {
            if (!resize(capacity * 2 + 1))
            {
                printf("Memory allocation failed\n");
                return false; // Memory allocation failed
            }
        }

        data[size] = value;
        size++;

        return true; // Adding succeeded
    }

    // operator[]: bounds-checked element access (const and mutable)
    const T &operator[](unsigned int index) const
    {
        if (index >= size)
        {
            return data[0];
        }

        return data[index];
    }
    T &operator[](unsigned int index) 
    {
        if (index >= size)
        {
            return data[0];
        }

        return data[index];
    }

    // get: safe retrieval returning default on OOB
    T get(unsigned int index)
    {
        if (index < 0 || index >= size)
        {
            return 0; // Return the default value
        }
        return data[index]; // Return the value at the index
    }

    // set: safe assignment with bounds check
    bool set(unsigned int index, T value)
    {
        if (index >= size)
        {
            return false;
        }

        data[index] = value;
        return true;
    }

    // print: debug helper to log contents, size, and capacity
    void print()
    {
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

// Enum _type
// Defines the categories of falling objects in the game
//   ROCK      – standard damaging object
//   POTION    – health restore
//   TIME_SLOW – slows drop speed temporarily
//   COIN      – bonus score
enum  _type {
    ROCK,
    POTION,
    TIME_SLOW,
    COIN,
};

// Struct rock_
// Represents a single falling object (rock or power‑up)
// Holds position, velocity, bitmap pointer, type flags, and status (draw/hit/missed)
struct rock_{
    double x_pos;
    double y_pos;
    bitmap *image;
    double velocity[2];
    bool draw;
    bool missed;
    bool hit;
    _type t;

    // Constructor:
    //  - Randomly choose image index and type based on POTION_RATE, TIME_SLOW_RATE, COIN_RATE
    //  - Initialize above-screen y position and random downward velocity
    rock_()
    {  
        int rock_i = rnd(5);

        y_pos = -bitmap_height("Rock_"+to_string(rock_i))*0.45;
        velocity[0] = 0;
        velocity[1] = rnd(20,100)/100.0;
        draw=true;
        missed=false;
        hit=false;
        float x_ = rnd();
        if (x_ < POTION_RATE)
        {
            t = POTION;
            image = &IMAGES[5];
        }
        else if (x_ < POTION_RATE + TIME_SLOW_RATE)
        {
            t = TIME_SLOW;
            image = &IMAGES[6];
        }
        else if (x_ < POTION_RATE + TIME_SLOW_RATE + COIN_RATE)
        {
            t = COIN;
            image = &IMAGES[7];
        }
        else
        {
            t = ROCK;
            image = &IMAGES[rock_i];
        }
        x_pos = rnd(-bitmap_width(*image)/2 + bitmap_width(*image)/15, SCREEN_WIDTH - bitmap_width(*image)/2 - bitmap_width(*image)/15)*1.0;
    }

    // draw_rock:
    //  - Render the bitmap at (x_pos,y_pos) scaled by 0.1
    //  - Move according to velocity, slowed if power‑up active
    void draw_rock(const bool &power_up)
    {
        draw_bitmap(*image, x_pos, y_pos, option_scale_bmp(0.1,0.1));
        if (power_up)
        {
            x_pos+=velocity[0]/10;
            y_pos+=velocity[1]/10;
        }
        else
        {
            x_pos+=velocity[0];
            y_pos+=velocity[1];
        }
    }

    // track_rock (debug):
    //  - Draw a collision circle around the rock’s center
    void track_rock()
    {
        double x = x_pos + bitmap_width(*image)/2;
        double y = y_pos + bitmap_height(*image)/2;
        double radius = bitmap_width(*image)/25;
        draw_circle(color_black(), x, y, radius);
    }
};

// Struct player_
// Holds player health, position (centered at bottom), and collision radius
struct player_
{
    double health;
    point_2d player_pos;
    double radius;

    // Constructor: set initial health, center bottom screen, fixed radius
    player_(int _health)
    {
        health = _health;
        player_pos.x = SCREEN_WIDTH/2;
        player_pos.y = SCREEN_HEIGHT - 50;
        radius = 50;
    }
};

// Struct stats_page
// Calculates post‑game stats (hits vs misses) and renders the Game Over menu
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

    // calc_stats: tally missed/hit from rock_history and compute dodge_accuracy
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
            }
            else if (rock->hit)
            {
                hit++;
            }
        }

        if (hit+missed==0)
        {
            dodge_accuracy = 0;
        }
        else
        {
            dodge_accuracy = ((missed/(hit+missed))*100);
        }
    }

    // mouse_on_button: hover detection for stats menu buttons
    bool mouse_on_button(double x)
    {
        return (mouse_x() > x && mouse_x() < x+SCREEN_WIDTH/5 && mouse_y() > SCREEN_HEIGHT*5/6 && mouse_y()< SCREEN_HEIGHT*5/6 + 100);
    }

    // draw_button: render a rectangular button with hover effect
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

    // draw_stats: main loop to display stats and capture user choice (EXIT vs MENU)
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

// Struct menu
// Renders the initial difficulty selection screen with EASY, MEDIUM, HARD, EXIT options
struct menu
{    
    menu()
    {
    }

    ~menu()
    {
    }

    // mouse_on_button: checks if cursor is over a menu button at vertical pos y
    bool mouse_on_button(double y)
    {
        return (mouse_x() > (SCREEN_WIDTH/3) && mouse_x() < 2 * (SCREEN_WIDTH/3) && mouse_y() > y && mouse_y() < y+100);
    }

    // draw_button: render menu button with hover feedback
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

    // draw_menu: display menu, handle clicks, return selected difficulty index
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
            draw_text("EXIT MENU",color_white(),FONT1, FONT_SIZE,SCREEN_WIDTH/2 -FONT_SIZE*3, SCREEN_HEIGHT/3 + 20);
            draw_text("EASY", color_white(),FONT1, FONT_SIZE, SCREEN_WIDTH/2  -FONT_SIZE*2, SCREEN_HEIGHT/3 + 140);
            draw_text("MEDIUM", color_white(),FONT1, FONT_SIZE, SCREEN_WIDTH/2  -FONT_SIZE*2, SCREEN_HEIGHT/3 + 260);
            draw_text("HARD", color_white(),FONT1, FONT_SIZE, SCREEN_WIDTH/2 -FONT_SIZE*2, SCREEN_HEIGHT/3 + 380);
            refresh_screen();
        }
        return 0;
    }
};

// Struct game_state
// Manages entire gameplay session:
//  - Player object, rock queue/history, timers, wind, power‑ups, scoring, difficulty
struct game_state
{
    player_ *player;
    bool over;
    
    unsigned int score;
    
    dynamic_array<rock_ *> *rock_history;
    dynamic_array<rock_ *> *rock_queue;
    
    unsigned int rock_release;
    unsigned int next_rock_time;
    unsigned long wind_change_time;

    timer game_clock;
    timer wind_clock;
    timer slow_clock;
    
    double max_health;
    
    double difficulty;
    unsigned long powerup_time;
    int wind;
    
    double rock_softness;//To make the rock hurt less
    double acceleration;//To increase falling rate

    // Constructor(difficulty):
    //  - Set up timers, difficulty scaling (health, acceleration), load images
    game_state(double _dif)
    {
        game_clock = create_timer("game_clock");
        wind_clock = create_timer("wind_clock");
        slow_clock = create_timer("slow_clock");
        powerup_time = 0;
        difficulty = _dif;

        if (_dif == 0)
        { 
            _dif = 0.0001;
        }

        rock_softness = 2 / (_dif+1);
        acceleration = 0.025 * (_dif);
        max_health = 30.0/(_dif);
        
        over = ((int)_dif == 0);

        player = (new player_(max_health));
        
        score = 0;
        wind_change_time = WIND_CHANGE_TIME;

        rock_history = new dynamic_array<rock_ *>(0);
        rock_queue = new dynamic_array<rock_ *>(0);

        rock_release = 0;
        next_rock_time = 1000;

        load_images();
    }

    // Destructor: clean up dynamic memory (player, rock arrays)
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

    // load_images: preload all rock and power‑up bitmaps into IMAGES array
    void load_images()
    {
        for (int i=0; i<8; i++)
        {
            IMAGES[i] = load_bitmap("Rock_"+to_string(i), "./" + to_string(i) + ".png");
        }
    }

    // populate_rock_queue: lazily generate rocks ahead of time (BUFFER + 2×released count)
    void populate_rock_queue()
    {
        for (int i = rock_queue->size; i < rock_release*2 + BUFFER; i++)
        {
            rock_ * new_rock = new rock_();
            rock_queue->add(new_rock);
        }
    }

    // remove_rock: flag rock as removed, record pointer in history
    void remove_rock(rock_ &rock)
    {
        rock_history->add(&rock);
        rock.draw = false;
    }

    // draw_rocks: render, update, and handle collisions/misses for active rocks
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
                rock->draw_rock(powerup_time>0);
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
                    switch (rock->t)
                    {
                        case ROCK:
                            player->health-=rock->velocity[1]/rock_softness;
                            rock->hit = true;
                            remove_rock(*rock);
                            break;
                        case POTION:
                            rock->draw = false;
                            if (player->health + max_health/8.0 > max_health)
                            {
                                player->health = max_health;
                            }
                            else
                            {
                                player->health += max_health/8.0;
                            }
                            break;
                        case TIME_SLOW:
                            rock->draw = false;
                            resume_timer(slow_clock);
                            if (powerup_time - timer_ticks(slow_clock) + 2000 > MAX_TIME_SLOW)
                            {
                                powerup_time = MAX_TIME_SLOW + timer_ticks(slow_clock);
                            }
                            else
                            {
                                powerup_time += 2000;
                            }
                            break;
                        case COIN:
                            rock->draw = false;
                            score+= difficulty;
                            break;
                    }
                    
                }
                
                else if (((*rock).y_pos + bitmap_height(*(*rock).image)/2)>=SCREEN_HEIGHT && !(*rock).missed)
                {
                    if (rock->t == ROCK)
                    {
                        rock->missed = true;
                        remove_rock(*rock);
                    }
                }
            } 
        }
    }

    // handle_mechanics: spawn timing, wind updates, power‑up expiration, death check
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
        if (powerup_time <= timer_ticks(slow_clock))
        {
            reset_timer(slow_clock);
            powerup_time = timer_ticks(slow_clock);
            pause_timer(slow_clock);
        }
        if (player->health <=0)
        {
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

    // draw_slow: render time‑slow power‑up bar at top
    void draw_slow()
    {
        double y_start = SCREEN_HEIGHT/10;
        double x_start = 3 * SCREEN_WIDTH/10;
        double width = SCREEN_WIDTH/4;
        double height = 15;

        double health_width = width * ((powerup_time - timer_ticks(slow_clock))/(float)MAX_TIME_SLOW);
        draw_text("Power Bar " , color_black(), FONT1, FONT_SIZE, x_start,y_start - 50 );

        fill_rectangle(color_white(), x_start, y_start, width, height);
        draw_rectangle(color_black(), x_start, y_start, width, height);
        fill_rectangle(color_light_blue(), x_start, y_start, health_width, height);
    }

    // draw_health: show player health bar and current score
    void draw_health()
    {
        double y_start = SCREEN_HEIGHT/10 - 5;
        double x_start = 6 * SCREEN_WIDTH/10;
        double width = SCREEN_WIDTH/4;
        double height = 20;

        double health_width = width * (player->health/max_health);

        draw_text("SCORE : " + to_string((int) score), color_black(), FONT1, FONT_SIZE, 50 ,y_start );

        fill_rectangle(color_red(), x_start, y_start, width, height);
        fill_rectangle(color_light_green(), x_start, y_start, health_width, height);
        if (powerup_time > 0)
        {
            draw_slow();
        }
    }

    // draw_player: render the player as a filled circle above health bar
    void draw_player()
    {
        fill_circle(color_black(), player->player_pos.x,player->player_pos.y - player->radius - 10,player->radius);
    }

    // render_game: main game loop (update, draw, timers) until over or quit
    void render_game()
    {
        start_timer(game_clock);
        start_timer(wind_clock);
        start_timer(slow_clock);
        while (!quit_requested())
        {   
            if (over)
            {
                break;
            }
            process_events();

            populate_rock_queue();

            handle_user_inputs();

            handle_mechanics();

            clear_screen(color_white());

            draw_rocks();

            draw_player();

            draw_health();

            refresh_screen();
        }
    }
};

// main: application entry point
//  - Loop: show menu → run game → show stats → exit or restart
int main()  
{   
    open_window("ROCK DODGER", SCREEN_WIDTH, SCREEN_HEIGHT);
    while (true)
    {
        menu *game_menu = new menu();

        game_state *game = new game_state((double)game_menu->draw_menu());
        delete game_menu;

        game->render_game();
      
        stats_page stats = stats_page(game->score, game->rock_history);        
        int user_opt = stats.draw_stats();

        delete game;
        if (user_opt == 0)
        {
           break;
        }
    } 
    return 0;
    write_line("Thanks for playing!");
}
