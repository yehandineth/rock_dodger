#include "splashkit.h"
#include <format>
using std::to_string;

const int SCREEN_HEIGHT = 720;
const int SCREEN_WIDTH = 1080;

const int MAX_ROCK_SPAWNS = 250;
const int NUM_IMAGES = 5;

const double ROCK_SOFTNESS = 2;//To make the rock hurt less

const int ACCELERATION = 25;//To increase falling rate

bitmap IMAGES[NUM_IMAGES];

struct rock_{
    double x_pos;
    double y_pos;
    bitmap *image;
    double velocity[2];
    bool draw = true;
    bool missed = false;
    bool hit = false;
};

struct player_
{
    double health = 10;
    point_2d player_pos;
    double radius;
};

struct rock_list
{
    rock_ rocks[MAX_ROCK_SPAWNS];
    int length;
};

struct rock_pointer_list
{
    rock_ *rocks[MAX_ROCK_SPAWNS];
    int length;
};

struct game_state
{
    player_ player;
    bool over = false;
    int score;
    timer game_clock;
    rock_pointer_list rock_history;
    rock_list rock_queue;
    int rock_release;
    long next_rock_time;
};

void load_images()
{
    for (int i=0; i<NUM_IMAGES; i++)
    {
        IMAGES[i] = load_bitmap("Rock_"+to_string(i), "./" + to_string(i) + ".png");
    }
}


rock_ generate_rock()
{
    int rock = rnd(NUM_IMAGES);
    double x = rnd(-bitmap_width("Rock_"+to_string(rock))*0.9 + 100, SCREEN_WIDTH - 100)*1.0;
    double y = -bitmap_height("Rock_"+to_string(rock))*0.45;
    while(true)
    {
        if (x + bitmap_width(IMAGES[rock])/2 < 50)
        {
            x += rnd(SCREEN_WIDTH)*1.0;
        }
        if (x + bitmap_width(IMAGES[rock])/2 > SCREEN_WIDTH-50)
        {
            x -= rnd(SCREEN_WIDTH)*1.0;
        }
        else
        {
            break;
        }
    }
    return rock_ {x, y, &IMAGES[rock], {0, rnd(20,100)/100.0}};
}

void populate_rock_queue(game_state &game)
{
    for (int i = 0; i < MAX_ROCK_SPAWNS; i++)
    {
        game.rock_queue.rocks[i] = generate_rock();
        game.rock_queue.length++;
    }
}

void draw_rock(rock_ &rock)
{
    draw_bitmap(*rock.image, rock.x_pos, rock.y_pos, option_scale_bmp(0.1,0.1));
    rock.x_pos+=rock.velocity[0];
    rock.y_pos+=rock.velocity[1];
}

void track_rock(const rock_ &rock)
{
    double x = rock.x_pos + bitmap_width(*rock.image)/2;
    double y = rock.y_pos + bitmap_height(*rock.image)/2;
    double radius = bitmap_width(*rock.image)/25;
    draw_circle(color_black(), x, y, radius);
}

void remove_rock(rock_pointer_list &history, rock_ &rock)
{
    history.rocks[history.length] = &rock;
    rock.draw = false;
    history.length++;
}

void draw_rocks(game_state &game)
{
    int j;
    for (int i = 0; i< game.rock_release; i++)
    {
        rock_ *rock = &game.rock_queue.rocks[i];
        if ((*rock).draw)
        {
            j++;//Track number of drawn rocks
            // track_rock(*rock); //Bounding boxes of rocks
            draw_rock(*rock);
        }
            if (circles_intersect(
                ((*rock).x_pos + bitmap_width(*(*rock).image)/2),
                ((*rock).y_pos + bitmap_height(*(*rock).image)/2),
                bitmap_width(*(*rock).image)/25,
                game.player.player_pos.x,
                game.player.player_pos.y,
                game.player.radius) && (*rock).draw)
                {
                    game.player.health-=(*rock).velocity[1]/ROCK_SOFTNESS;
                    (*rock).draw = true;
                    remove_rock(game.rock_history, *rock);
                }
        else if (((*rock).y_pos + bitmap_height(*(*rock).image)/2)>=SCREEN_HEIGHT && !(*rock).missed)
        {
            // printf("WORKS\n");
            (*rock).missed = true;
            // printf("WORKS\n");
            remove_rock(game.rock_history, *rock);
        }
    } 
    // printf("%d\n", j);
}

void handle_mechanics(game_state &game)
{
    if (timer_ticks(game.game_clock)>game.next_rock_time && game.rock_release < game.rock_queue.length)
    {
        game.rock_release++;
        reset_timer(game.game_clock);
        game.next_rock_time = rnd(500, 1500)/(1 + (game.rock_release/ACCELERATION));
    }
    if (game.player.health <=0)
    {
        //LOSE MENU
        game.over = true;
    }
    if (game.rock_history.length == MAX_ROCK_SPAWNS)
    {
        //WIN MENU
        game.over = true;
    }
}

void handle_user_inputs(game_state &game)
{
    if (key_down(Q_KEY))
    {
        game.over = true;
    }
    // if (key_down(SPACE_KEY))//For debugging
    // {
    //     printf("%d : %d : %d : %d: %f\n", timer_ticks(game.game_clock) ,game.next_rock_time, game.rock_release , game.rock_queue.length ,game.rock_queue.rocks[game.rock_release-2].x_pos + bitmap_width(*game.rock_queue.rocks[game.rock_release-2].image)/2);
    // }
    if (key_down(LEFT_KEY) && game.player.player_pos.x >= game.player.radius)
    {
        game.player.player_pos.x -= 0.5;
    }
    if (key_down(RIGHT_KEY) && game.player.player_pos.x <= (SCREEN_WIDTH - game.player.radius))
    {
        game.player.player_pos.x += 0.5;
    }
  
}



void draw_text(game_state &game)
{
    draw_text("Health Remaining : " + to_string((int) game.player.health), color_black(), 20 ,20 );
}

void draw_player(const player_ &player)
{
    fill_circle(color_black(), player.player_pos.x,player.player_pos.y - player.radius - 10,player.radius);
}

void render_game(game_state &game)
{
    while (!quit_requested())
    {   
        if (game.over)
        {
            return;
        }
        process_events();
        handle_user_inputs(game);
        handle_mechanics(game);
        clear_screen(color_white());
        draw_rocks(game);
        draw_player(game.player);
        draw_text(game);
        refresh_screen();
    }
}

game_state create_game()
{
    rock_pointer_list rock_history = {{},0};
    rock_list rock_queue = {{},0};
    game_state game = {
        {10, point_2d {SCREEN_WIDTH/2, SCREEN_HEIGHT}, 50},
        false,
        0,
        create_timer("game_clock"),
        rock_history,
        rock_queue,
        0,
        1000,
    };
    return game;
}

int main()
{   
    open_window("Map Editor", SCREEN_WIDTH, SCREEN_HEIGHT);
    load_images();
    game_state game = create_game();
    populate_rock_queue(game);
    start_timer(game.game_clock);
    render_game(game);

    return 0;
}