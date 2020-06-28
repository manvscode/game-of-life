#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <xtd/console.h>
#include <xtd/memory.h>
#include <xtd/time.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

typedef struct app {
	int rows;
	int cols;
	bool quit;
	bool* world;
} app_t;

void world_clear(app_t* app)
{
	for( int y = 0; y < app->rows; y += 1 )
	{
		for( int x = 0; x < app->cols; x += 1 )
		{
			int idx = y * (app->cols + 1) + x;
			app->world[ idx ] = false;
		}
	}
}

void world_draw(app_t* app)
{
	console_hide_cursor( stdout );
	console_goto( stdout, 0, 0 );
	console_bg_color_256( stdout, CONSOLE_COLOR256_BLACK );

	for( int y = 0; y <= app->rows; y += 1 )
	{
		for( int x = 0; x <= app->cols; x += 1 )
		{
			int idx = y * (app->cols + 1) + x;
			bool is_occupied = app->world[ idx ];
			if( is_occupied )
			{
				console_bg_color_256( stdout, CONSOLE_COLOR256_GREEN );
			}
			console_goto( stdout, x, y );
			fprintf( stdout, " ");

			if( is_occupied )
			{
				console_bg_color_256( stdout, CONSOLE_COLOR256_BLACK );
			}
		}
	}
}

#define make_pattern(a,b,c,d,e,f,g,h,i) (\
	((a) << 8) | \
	((b) << 7) | \
	((c) << 6) | \
	((d) << 5) | \
	((e) << 4) | \
	((f) << 3) | \
	((g) << 2) | \
	((h) << 1) | \
	((i) << 0))

typedef unsigned short pattern_t;

static const pattern_t PATTERNS[] = {
	make_pattern( // Block
		1, 1, 1,
		1, 1, 1,
		1, 1, 1
	),
	make_pattern( // Block
		0, 0, 0,
		0, 1, 1,
		0, 1, 1
	),
	make_pattern( // Block
		0, 0, 0,
		1, 1, 0,
		1, 1, 0
	),
	make_pattern( // Block
		1, 1, 0,
		1, 1, 0,
		0, 0, 0
	),
	make_pattern( // Block
		0, 1, 1,
		0, 1, 1,
		0, 0, 0
	),
	make_pattern( // Oscillator
		0, 0, 0,
		1, 1, 1,
		0, 0, 0
	),
	make_pattern( // Oscillator
		1, 0, 0,
		0, 1, 0,
		0, 0, 1
	),
	make_pattern( // Oscillator
		0, 1, 0,
		0, 1, 0,
		0, 1, 0
	),
	make_pattern( // Oscillator
		0, 0, 1,
		0, 1, 0,
		1, 0, 0
	),
	make_pattern( // Glider
		0, 1, 0,
		0, 0, 1,
		1, 1, 1
	),
	make_pattern( // Glider
		1, 0, 0,
		1, 0, 1,
		1, 1, 0
	),
	make_pattern( // Glider
		1, 1, 1,
		1, 0, 0,
		0, 1, 0
	),
	make_pattern( // Glider
		0, 0, 1,
		1, 0, 1,
		0, 1, 1
	),
	make_pattern( // X
		1, 0, 1,
		0, 0, 0,
		1, 0, 1
	),
	make_pattern( // X
		1, 0, 1,
		0, 1, 0,
		1, 0, 1
	),
};
static const size_t PATTERNS_COUNT = sizeof(PATTERNS) / sizeof(PATTERNS[0]);


void world_seed(app_t* app)
{
	int count = (app->cols / 2);
	for( int i = 0; i < count; i += 1)
	{
		srand(clock());
		int x = 1 + (rand() % (app->cols - 1));
		int y = 1 + (rand() % (app->rows - 1));

		pattern_t pattern = PATTERNS[ rand() % PATTERNS_COUNT ];

		for( int i = 0; i <= 2; i++)
		{
			for( int j = 0; j <= 2; j++)
			{
				int bit = 3 * i + j;
				int _x = x + j;
				int _y = y + i;
				int idx = _y * (app->cols + 1) + _x;
				app->world[ idx ] = pattern & (1 << (8 - bit));

			}
		}
	}
}

void world_resize(app_t* app)
{
	int new_rows = 0;
	int new_cols = 0;
	console_size(stdout, &new_rows, &new_cols);

	size_t size = sizeof(*app->world) * app->rows * app->cols;
	bool* new_world = malloc( size );

	if( new_world )
	{
		if( app->world )
		{
			// copy the world.
			for( int y = 0; y <= app->rows; y += 1 )
			{
				for( int x = 0; x <= app->cols; x += 1 )
				{
					int idx_new = y * (new_cols + 1) + x;
					int idx_old = y * (app->cols + 1) + x;
					new_world[ idx_new ] = app->world[ idx_old ];
				}
			}

			free( app->world );
		}

		app->world = new_world;
		app->rows  = new_rows;
		app->cols  = new_cols;
	}
	else
	{
		app->quit = true;
	}
}

int world_sum_at(app_t* app, int x, int y, bool alive)
{
	int sum = 0;

	for( int i = -1; i <= 1; i++ )
	{
		for( int j = -1; j <= 1; j++ )
		{
			int _x = x + j;
			int _y = y + i;

			if( i == 0 && j == 0 ) continue;
			else if( _x < 0 ||  _y < 0 ) continue;
			else if( _x > app->cols ||  _y > app->rows ) continue;

			int index = _y * (app->cols + 1) + _x;

			if( app->world[ index ] == alive )
			{
				sum += 1;
			}
		}
	}

	return sum;
}

void world_update(app_t* app)
{
	for( int y = 0; y <= app->rows; y++ )
	{
		for( int x = 0; x <= app->cols; x++ )
		{
			int index = y * (app->cols + 1) + x;
			bool is_alive = app->world[ index ];
			int sum = world_sum_at( app, x, y, true );

			if( is_alive )
			{
				if( sum != 2 && sum != 3 )
				{
					app->world[ index ] = false;
				}
			}
			else
			{
				if( sum == 3 )
				{
					app->world[ index ] = true;
				}
			}
		}
	}
}


int main(int argc, char* argv[])
{
	app_t app = (app_t) {
		.rows = 0,
		.cols = 0,
		.quit = false,
		.world = NULL,
	};

	sigset_t signals;
	sigset_t blocked_signals;
	sigset_t pending_signals;

	sigfillset(&signals);
	sigemptyset(&blocked_signals);
	sigaddset(&blocked_signals, SIGINT);
	sigaddset(&blocked_signals, SIGQUIT);
	sigaddset(&blocked_signals, SIGWINCH);

	sigprocmask(SIG_BLOCK, &blocked_signals, &signals);
	sigemptyset(&pending_signals);

	console_clear_screen_all( stdout );

	world_resize(&app);
	world_clear(&app);
	world_seed(&app);

	while( !app.quit )
	{
		if( !sigpending( &pending_signals ) )
		{
			if(sigismember(&pending_signals, SIGQUIT) ||
			   sigismember(&pending_signals, SIGINT))
			{
				console_show_cursor( stdout );
				console_clear_screen_all( stdout );
				console_reset( stdout );
				app.quit = true;
			}
			if(sigismember(&pending_signals, SIGWINCH))
			{
				world_resize(&app);
			}
		}

		world_draw(&app);
		time_msleep(100);
		world_update(&app);
	}

	if( app.world) free(app.world);
	fprintf(stdout, "Done.\n");

	return 0;
}
