// SDL libraries
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

// Standard libraries
#include <stdlib.h>
#include <stdio.h>

// Helper libraries
#include <stdbool.h>
#include <time.h>

#pragma region Structs and ENUMs

typedef struct Enemy
{
	float px, py; // Coordinates

	int killReward; // Amount of points player gets for kiling enemy
	float shootTimer; // Cooldown for shooting bullets
} Enemy;

typedef struct Player
{
	float px, py; // Coordinates

	int score; // Current score
	int hiScore; // High-score

	int livesLeft; // Amount of lives left until game resets
	float shootTimer; // Cooldown for shooting bullets
} Player;

typedef struct Bullet
{
	float px, py; // Coordinates

	int vy; // Y-velocity, used for determining the direction (up or down) of the bullet.

} Bullet;

typedef enum ParticleTypes {BULLET_EXPLOSION, SHIP_EXPLOSION} ParticleTypes; // Types of particles

typedef struct Particle 
{
	float px, py; // Coordinates

	float lifetime; // Lifetime of particle (how long particle stays on screen)

	ParticleTypes particleType; // The type of particle
} Particle;

#pragma endregion

#pragma region Globals and defines

// Window size
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

// Maximums for enemies, projectiles (bullets) and particles
#define MAX_ENEMIES 55
#define MAX_PROJECTILES 20
#define MAX_PARTICLES 20

// Speed variables for all entities
unsigned const int PLAYER_SPEED = 200;
unsigned const int ENEMY_SPEED = 20;
unsigned const int BULLET_SPEED = 275;

// Enemy speed multiplier, used for increasing speed every time enemies change direciton
float ENEMY_SPEED_MULT = 1.0f;

// Player size
unsigned const int PLAYER_WIDTH = 20;
unsigned const int PLAYER_HEIGHT = 20;

// Enemy size
unsigned const int ENEMY_WIDTH = 30;
unsigned const int ENEMY_HEIGHT = 30;

// Bullet (projectile) size
unsigned const int BULLET_WIDTH = 10;
unsigned const int BULLET_HEIGHT = 15;

// Particle size
unsigned const int PARTICLE_WIDTH = 20;
unsigned const int PARTICLE_HEIGHT = 20;

// Player object
Player player;

// Global pointers for storing data of enemies, bullets and particles
Enemy* enemies[MAX_ENEMIES] = { NULL };
Bullet* bullets[MAX_PROJECTILES] = { NULL };
Particle* particles[MAX_PARTICLES] = { NULL };

// Global pointers for SDL
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Global pointers for sound effects
Mix_Chunk* shoot_sound = NULL;
Mix_Chunk* hit_sound = NULL;

// Global pointers of the textures of the entities
SDL_Texture* enemy_tex = NULL;
SDL_Texture* particle_tex = NULL;
SDL_Texture* bullet_tex = NULL;

// Global pointer for the game and menu font 
TTF_Font* game_font;
TTF_Font* menu_font;

// Current enemy direction and wave number
int enemyDir = 1;
int currentWave = 1;

// Boolean used for determining if the game is over
static bool gameOver = false;

static bool playGame = false;

#pragma endregion

#pragma region Function forward declarations

// Initialization and exit
bool init(void);
void exitProgram(void);

// Main update methods
void update(float delta);

// Player methods
void createPlayer(Player* player);
void updatePlayer(Player* player, float delta);

// Enemy methods
void createEnemies(void);
void updateEnemies(float delta);
void enemyShoot(float delta);

// Bullet methods
void createBullet(int px, int py, int dir);
void updateBullets(float delta);

// Particle methods
void createParticle(Particle particle);
void updateParticles(float delta);

// Collision checking and game state checking
void checkBulletCollisions(Player* player);
void checkGameState(Player* player);

// Rendering
void drawText(const char* text, TTF_Font* font, SDL_Color color, int px, int py);
void renderEntities(void);
void renderStats(void);

// Freeing memory used by global pointers
void freeEnemies(void);
void freeBullets(void);
void freeParticles(void);

// Menu functions
void renderMenu(void);
void checkGameStart(void);

#pragma endregion	

int main(int argc, char* argv[])
{
	// If initialization is unsuccessful, exit the program
	if (!init()) {
		exitProgram();
	}

	atexit(exitProgram);

	SDL_Event event;
	bool quit = false;

	Uint32 lastFrame = SDL_GetTicks();

	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			}
		}

		Uint32 curFrame = SDL_GetTicks();

		Uint32 difference = curFrame - lastFrame;
		float delta = difference / 1000.0f;

		lastFrame = curFrame;

		update(delta);
	}

	return 0;
}

// Function that initializes everything for the program to run correctly
bool init(void)
{
	bool success = true;

	// Initialize base SDL library
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("Couldn't initialize SDL: %s", SDL_GetError());
		success = false;
	}

	// Initialize the TTF library (text rendering)
	if (TTF_Init() != 0) {
		printf("Couldn't initialize SDL: %s", TTF_GetError());
		success = false;
	}

	// Initialize the Mix library (audio playback)
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		success = false;
	}
	
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
		printf("SDL_image could not initialize! SDL_image Error: %s", IMG_GetError());
		success = false;
	}

	// Set render scale quality to 0 for the crispiest pixel art
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	// Load the game and menu fonts
	game_font = TTF_OpenFont("rubik-reg.ttf", 18);
	menu_font = TTF_OpenFont("rubik-reg.ttf", 64);

	// Load the two sound effects
	shoot_sound = Mix_LoadWAV("shoot_sound.wav");
	hit_sound = Mix_LoadWAV("hit_sound.wav");

	enemy_tex = IMG_LoadTexture(renderer, "enemy.png");
	bullet_tex = IMG_LoadTexture(renderer, "bullet.png");

	// Create a window and a renderer
	window = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Create player and enemies
	createPlayer(&player);
	createEnemies();

	// Set a seed for pseudo-random number generation
	srand((unsigned int)time(0));

	// Return bool indicating the success of initailizing everything
	return success;
}

// Function that gets called when the program exits
void exitProgram(void)
{
	// Free memory
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	TTF_CloseFont(game_font);

	SDL_DestroyTexture(enemy_tex);
	SDL_DestroyTexture(particle_tex);
	SDL_DestroyTexture(bullet_tex);

	Mix_FreeChunk(hit_sound);
	Mix_FreeChunk(shoot_sound);

	freeBullets();
	freeEnemies();
	freeParticles();

	// Quit libraries
	Mix_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

// Main update function that calls all other function
void update(float delta) 
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	if(playGame) 
	{
		updatePlayer(&player, delta);
		updateBullets(delta);
		updateEnemies(delta);
		updateParticles(delta);

		enemyShoot(delta);

		checkBulletCollisions(&player);
		checkGameState(&player);

		renderStats();
		renderEntities();
	}
	else 
	{
		renderMenu();
		checkGameStart();
	}

	SDL_RenderPresent(renderer);
}

// Function for creating the player
void createPlayer(Player* player)
{
	// Initialize variables
	player->px = WINDOW_WIDTH / 2;
	player->py = WINDOW_HEIGHT - (WINDOW_HEIGHT / 14);
	
	player->score = 0;
	player->hiScore = 0;
	
	player->livesLeft = 3;
	player->shootTimer = 0.0;
}

// Function for updating the player
void updatePlayer(Player* player, float delta)
{
	// Get keyboard state
	const Uint8* state = SDL_GetKeyboardState(NULL);

	// Move right
	if (state[SDL_SCANCODE_RIGHT])
	{
		player->px += PLAYER_SPEED * delta;
	}

	// Move left
	if (state[SDL_SCANCODE_LEFT])
	{
		player->px -= PLAYER_SPEED * delta;
	}

	// Shoot
	if (state[SDL_SCANCODE_SPACE])
	{
		if (player->shootTimer <= 0)
		{
			createBullet((int)player->px, (int)player->py, -1);
			Mix_PlayChannel(-1, shoot_sound, 0);

			player->shootTimer = 1.0;
		}
	}

	player->shootTimer -= delta;
}

// Function for creating a new set of enemies
void createEnemies(void)
{
	for(unsigned int i = 0; i < MAX_ENEMIES; i++) 
	{
		// Allocate memory
		enemies[i] = (Enemy*)malloc(sizeof(Enemy));

		// Get row and column
		int px = i % 11;
		int py = i / 11;

		// Initialize variables if malloc succeeded
		if (enemies[i])
		{			
			enemies[i]->px = (float)px * ENEMY_WIDTH + (10.0f * (float)px) + 100;
			enemies[i]->py = (float)py * ENEMY_HEIGHT + (10.0f * (float)py) + 100;
			enemies[i]->killReward = 10 * currentWave;
			enemies[i]->shootTimer = max(0.20f, 1 - (0.05f * currentWave));
		}
	}
}

// Function for updating enemies
void updateEnemies(float delta)
{
	for (unsigned int i = 0; i < MAX_ENEMIES; i++)
	{
		// If enemy exists...
		if (enemies[i] != NULL)
		{
			// Move right if 1, else left
			if (enemyDir == 1)
				enemies[i]->px += (float)ENEMY_SPEED * ENEMY_SPEED_MULT * delta;
			else
				enemies[i]->px -= (float)ENEMY_SPEED * ENEMY_SPEED_MULT * delta;

			// If enemies get too close to player, end the game
			if (enemies[i]->py > WINDOW_HEIGHT - PLAYER_HEIGHT * 2 - ENEMY_HEIGHT)
				gameOver = true;
		}
	}

	for (unsigned int i = 0; i < MAX_ENEMIES; i++)
	{
		// if enemy exists...
		if (enemies[i] != NULL)
		{
			// If an enemy is too close to either edge, change to direction of every enemey
			if (enemies[i]->px < 0 + (ENEMY_WIDTH) || enemies[i]->px > WINDOW_WIDTH - (2 * ENEMY_WIDTH))
			{
				// Inverse direction
				enemyDir = -enemyDir;

				// Move enemies down by 1/4 of their height
				for (unsigned int i = 0; i < MAX_ENEMIES; i++)
					if (enemies[i] != NULL)
						enemies[i]->py += ENEMY_HEIGHT / 4;
				
				ENEMY_SPEED_MULT += 0.025f;

				// break so that this procedure only happens once
				break;
			}
		}
	}
}

// Function that makes enemies shoot on a semi-random basis
void enemyShoot(float delta)
{
	int index = MAX_ENEMIES - 1;
	int found[] = { 0,0,0,0,0,0,0,0,0,0,0 };

	for (int i = 0; i < 5; i++)
	{
		for(int j = 0; j < 11; j++) 
		{
			if (enemies[index] != NULL && found[j] != 1)
			{
				found[j] = 1;

				if (rand() % 5000 < 10 && enemies[index]->shootTimer <= 0)
				{
					createBullet((int)enemies[index]->px, (int)enemies[index]->py, 1);


					Mix_PlayChannel(-1, shoot_sound, 0);

					enemies[index]->shootTimer = 1;
				}

				enemies[index]->shootTimer -= delta;
			}

			index--;
		}
	}
}

// Function for creating a new bullet
void createBullet(int px, int py, int dir)
{
	int w_offset = 0, h_offset = 0;

	if (dir == -1) {
		w_offset = PLAYER_WIDTH / 2;
		h_offset = PLAYER_HEIGHT / 2;
	}
	else if (dir == 1) {
		w_offset = ENEMY_WIDTH / 2;
		h_offset = ENEMY_HEIGHT / 2;
	}

	for (unsigned int i = 0; i < MAX_PROJECTILES; i++)
	{
		if (bullets[i] == NULL)
		{
			bullets[i] = (Bullet*)malloc(sizeof(Bullet));

			if (bullets[i])
			{
				bullets[i]->px = (float)px + w_offset;
				bullets[i]->py = (float)py + h_offset;
				bullets[i]->vy = dir;
			}

			break;
		}
	}
}

// Function for updating all bullets
void updateBullets(float delta)
{
	for (unsigned int i = 0; i < MAX_PROJECTILES; i++)
	{
		if (bullets[i] != NULL)
		{
			bullets[i]->py += (float)bullets[i]->vy * (float)BULLET_SPEED * delta;

			if (bullets[i]->py < 0 || bullets[i]->py > WINDOW_HEIGHT - BULLET_HEIGHT)
			{
				Particle particle = {
					.px = bullets[i]->px,
					.py = bullets[i]->py,
					.lifetime = 0.5f,
					.particleType = BULLET_EXPLOSION,
				};

				createParticle(particle);

				free(bullets[i]);
				bullets[i] = NULL;
				
				Mix_PlayChannel(-1, hit_sound, 0);
			}
		}
	}
}

// Function for creating a new particle
void createParticle(Particle particle) 
{
	// Find an unused spot in the particles pointer, allocate memory and assign new value to the particle in question
	for (unsigned int i = 0; i < MAX_PARTICLES; i++)
	{
		if (particles[i] == NULL)
		{
			particles[i] = (Particle*)malloc(sizeof(Particle));

			if (particles[i])
			{
				particles[i]->px = particle.px;
				particles[i]->py = particle.py;
				particles[i]->lifetime = particle.lifetime;
				particles[i]->particleType = particle.particleType;
			}

			break;
		}
	}
}

// Function for updating particles
void updateParticles(float delta) 
{
	// Loop through all existing particles, check if they should be destroyed and decrement their lifetime counter
	for (unsigned int i = 0; i < MAX_PARTICLES; i++)
	{
		if (particles[i] != NULL && particles[i])
		{
			particles[i]->lifetime -= delta;

			if(particles[i]->lifetime <= 0) 
			{
				free(particles[i]);
				particles[i] = NULL;
			}
		}
	}
}

// Function for cheking bullet collisions
void checkBulletCollisions(Player* player)
{
	SDL_Rect playerRect = {
		.x = (int)player->px,
		.y = (int)player->py,
		.w = PLAYER_WIDTH,
		.h = PLAYER_HEIGHT
	};

	for (unsigned int i = 0; i < MAX_PROJECTILES; i++)
	{
		if (bullets[i] != NULL)
		{
			SDL_Rect bulletRect = {
				.x = (int)bullets[i]->px,
				.y = (int)bullets[i]->py,
				.w = BULLET_WIDTH,
				.h = BULLET_HEIGHT
			};

			for (unsigned int j = 0; j < MAX_ENEMIES; j++)
			{
				if (enemies[j] != NULL)
				{
					SDL_Rect enemyRect = {
					.x = (int)enemies[j]->px,
					.y = (int)enemies[j]->py,
					.w = ENEMY_WIDTH,
					.h = ENEMY_HEIGHT
					};

					if (bullets[i])
					{
						if (SDL_HasIntersection(&bulletRect, &enemyRect) && bullets[i]->vy != 1)
						{
							SDL_Log("Enemy hit");
							Mix_PlayChannel(-1, hit_sound, 0);

							player->score += enemies[j]->killReward;

							Particle particle = {
								.px = (float)enemyRect.x,
								.py = (float)enemyRect.y,
								.lifetime = 0.5f,
								.particleType = SHIP_EXPLOSION,
							};

							createParticle(particle);

							free(enemies[j]);
							enemies[j] = NULL;

							free(bullets[i]);
							bullets[i] = NULL;
						}
					}
				}
			}

			if (bullets[i])
			{
				if (SDL_HasIntersection(&bulletRect, &playerRect) && bullets[i]->vy != -1)
				{
					Particle particle = {
						.px = bullets[i]->px,
						.py = bullets[i]->py,
						.lifetime = 0.5f,
						.particleType = SHIP_EXPLOSION,
					};

					createParticle(particle);

					free(bullets[i]);
					bullets[i] = NULL;

					Mix_PlayChannel(-1, hit_sound, 0);

					player->livesLeft -= 1;

					if (player->livesLeft <= 0)
						gameOver = true;

					player->px = WINDOW_WIDTH / 2;
					player->py = WINDOW_HEIGHT - (WINDOW_HEIGHT / 14);
				}
			}
		}
	}
}

// Function for ending the game and spawning new waves
void checkGameState(Player* player)
{
	// If game is over, set a new highscore, create a new player and create new enemies
	if (gameOver)
	{
		int tempScore = player->hiScore;
		if (player->score > player->hiScore)
		{
			tempScore = player->score;
		}

		createPlayer(player);
		player->hiScore = tempScore;

		freeEnemies();
		freeBullets();

		createEnemies();

		gameOver = false;
	}
	
	// If no more enemies exist, spawn a new wave
	bool spawnNextWave = true;
	for (unsigned int i = 0; i < MAX_ENEMIES; i++)
		if (enemies[i] != NULL) spawnNextWave = false;

	// Increment wave counter and spawn new enemies. Reset player lives to three.
	if (spawnNextWave)
	{
		currentWave += 1;

		freeEnemies();
		createEnemies();

		player->livesLeft = 3;
	}
}

// Function for rendering the current stats on screen
void renderStats(void)
{
	SDL_Color color = { 255,255,255 };

	// Create buffer for text
	char buffer[20];

	// Use snprintf to concatenate text and integers to a single buffer
	snprintf(buffer, sizeof buffer, "%s%d", "Score: ", player.score);

	drawText(buffer, game_font, color, 10, 15);

	// The same thing continues on for the next lines...

	snprintf(buffer, sizeof buffer, "%s%d", "High score: ", player.hiScore);

	drawText(buffer, game_font, color, 10, 35);

	snprintf(buffer, sizeof buffer, "%s%d", "Lives: ", player.livesLeft);

	drawText(buffer, game_font, color, 520, 15);
	
	snprintf(buffer, sizeof buffer, "%s%d", "Wave: ", currentWave);

	drawText(buffer, game_font, color, 520, 35);
}

// Function for rendering every type of entity in the game
void renderEntities(void) 
{
	// Set drawcolor to white
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	// Create rectangle based on the player's coordinates and size
	SDL_Rect rect = {
		.x = (int)player.px,
		.y = (int)player.py,
		.w = PLAYER_WIDTH,
		.h = PLAYER_HEIGHT
	};

	// Render given rectangle
	SDL_RenderFillRect(renderer, &rect);

	enemy_tex = IMG_LoadTexture(renderer, "enemy.png");

	for (unsigned int i = 0; i < MAX_ENEMIES; i++)
	{
		// If enemy exists...
		if (enemies[i] != NULL)
		{
			// Create rectangle based on the coordinates and size of the enemy
			SDL_Rect rect = {
			.x = (int)enemies[i]->px,
			.y = (int)enemies[i]->py,
			.w = ENEMY_HEIGHT,
			.h = ENEMY_WIDTH
			};

			// Render the given enemy texture at the coordinates of the given rectangle
			SDL_RenderCopy(renderer, enemy_tex, NULL, &rect);
		}
	}

	bullet_tex = IMG_LoadTexture(renderer, "bullet.png");

	for (unsigned int i = 0; i < MAX_PROJECTILES; i++)
	{
		// If bullet exists...
		if (bullets[i] != NULL)
		{
			// Create rectangle based on the coordinates and size of the bullet
			SDL_Rect rect = {
			.x = (int)bullets[i]->px,
			.y = (int)bullets[i]->py,
			.w = BULLET_WIDTH,
			.h = BULLET_HEIGHT
			};

			// Render rectangle
			SDL_RenderCopy(renderer, bullet_tex, NULL, &rect);
		}
	}

	for(unsigned int i = 0; i < MAX_PARTICLES; i++) 
	{
		if(particles[i] != NULL) 
		{
			SDL_Rect rect = {
			.x = (int)particles[i]->px,
			.y = (int)particles[i]->py,
			.w = PARTICLE_WIDTH,
			.h = PARTICLE_HEIGHT
			};

			switch (particles[i]->particleType)
			{
				case BULLET_EXPLOSION:
					particle_tex = IMG_LoadTexture(renderer, "bullet_destroy_particle.png");
					break;
				case SHIP_EXPLOSION:
					particle_tex = IMG_LoadTexture(renderer, "ship_destroy_particle.png");
					break;
			}

			SDL_RenderCopy(renderer, particle_tex, NULL, &rect);
		}
	}

	// Free memory
	SDL_DestroyTexture(particle_tex);
	SDL_DestroyTexture(enemy_tex);
	SDL_DestroyTexture(bullet_tex);
}

// Helper function for drawing text
void drawText(const char* text, TTF_Font* font, SDL_Color color, int px, int py)
{
	// Create a surface and texture based on the arguments that have been passed in
	SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, color);
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

	// Get the size of the texture
	int w, h;
	SDL_QueryTexture(textTexture, NULL, NULL, &w, &h);

	SDL_Rect textRect = { .x = px, .y = py, .w = w, .h = h };

	// Render text to screen
	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	// Free memory used by the surface and the texture
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
}

// Free memory used by enemies
void freeEnemies(void) 
{
	for(unsigned int i = 0; i < MAX_ENEMIES; i++) 
	{
		free(enemies[i]);
		enemies[i] = NULL;
	}
}

// Free memory used by bullets
void freeBullets(void) 
{
	for (unsigned int i = 0; i < MAX_PROJECTILES; i++)
	{
		free(bullets[i]);
		bullets[i] = NULL;
	}
}
	
// Free memory used by particles
void freeParticles(void) 
{
	for (unsigned int i = 0; i < MAX_PARTICLES; i++)
	{
		free(particles[i]);
		particles[i] = NULL;
	}
}

void renderMenu(void) 
{
	// Title
	SDL_Color white = { 255,255,255 };
	drawText("SPACE INVADERS", menu_font, white, WINDOW_WIDTH / 2 - 270, 35);

	// Draw button text
	drawText("Press space to play", menu_font, white, WINDOW_WIDTH / 2 - 85, WINDOW_HEIGHT / 2 - 35);	
}

void checkGameStart(void) 
{
	const Uint8* state = SDL_GetKeyboardState(NULL);

	if(state[SDL_SCANCODE_SPACE]) 
	{
		playGame = true;
		SDL_Delay(500);
	}
}
