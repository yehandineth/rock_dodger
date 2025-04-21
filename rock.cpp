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

const double ROCK_SOFTNESS = 2;//To make the rock hurt less

const int ACCELERATION = 25;//To increase falling rate

bitmap IMAGES[NUM_IMAGES];

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
        delete this;
    }
    void draw_rock()
    {
        draw_bitmap(*image, x_pos, y_pos, option_scale_bmp(0.1,0.1));
        track_rock();//For debugging purposes
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
        delete this;
    }
    
};

struct game_state
{
    player_ *player;
    bool over;
    int score;
    timer game_clock;
    dynamic_array<rock_ *> *rock_history;
    dynamic_array<rock_ *> *rock_queue;
    int rock_release;
    long next_rock_time;
    int last_added_rock;

    game_state ()
    {
        player = (new player_(100));
        over = false;
        score = 0;

        rock_history = new dynamic_array<rock_ *>(0);
        rock_queue = new dynamic_array<rock_ *>(0);

        game_clock = create_timer("game_clock");

        rock_release = 0;
        next_rock_time = 1000;

        load_images();
    }
    ~game_state ()
    {
        delete player;
        delete rock_history;
        delete rock_queue;
        delete this;
    }

    void load_images()
    {
        for (int i=0; i<NUM_IMAGES; i++)
        {
            IMAGES[i] = load_bitmap("Rock_"+to_string(i), "./" + to_string(i) + ".png");
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
                if (circles_intersect(
                    (rock->x_pos + (double) bitmap_width(*rock->image)/2),
                    (rock->y_pos + (double) bitmap_height(*rock->image)/2),
                    bitmap_width(*rock->image)/25,
                    player->player_pos.x,
                    player->player_pos.y,
                    player->radius)
                )
                {
                    player->health-=rock->velocity[1]/ROCK_SOFTNESS;
                    rock->hit = true;
                    remove_rock(*rock);
                }
                
                else if (((*rock).y_pos + bitmap_height(*(*rock).image)/2)>=SCREEN_HEIGHT && !(*rock).missed)
                {
                    // printf("WORKS\n");
                    rock->missed = true;
                    // printf("WORKS\n");
                    remove_rock(*rock);
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
            next_rock_time = rnd(500, 1500)/(1 + (rock_release/ACCELERATION));
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
        // if (key_down(SPACE_KEY))//For debugging
        // {
        //     printf("%d : %d : %d : %d: %f\n", timer_ticks(game_clock) ,next_rock_time, rock_release , rock_queue.length ,rock_queue.rocks[rock_release-2].x_pos + bitmap_width(*rock_queue.rocks[rock_release-2].image)/2);
        // }
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
        draw_text("Health Remaining : " + to_string((int) player->health), color_black(), 20 ,20 );
    }

    void draw_player()
    {
        fill_circle(color_black(), player->player_pos.x,player->player_pos.y - player->radius - 10,player->radius);
    }

    void render_game()
    {
        start_timer(game_clock);
        while (!quit_requested())
        {   
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
    open_window("Map Editor", SCREEN_WIDTH, SCREEN_HEIGHT);
    game_state *game = new game_state();
    game->render_game();
    delete game;
    return 0;
}