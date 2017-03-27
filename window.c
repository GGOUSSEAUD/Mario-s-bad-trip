q/*!\file window.c
 *
 * \brief Utilisation de la SDL2 et d'OpenGL 3+
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date February 03 2014
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_image.h>

#include <gl4du.h>
#include <SDL_mixer.h>
/*
 * Prototypes des fonctions statiques contenues dans ce fichier C
 */

static SDL_Window * initWindow(int w, int h, SDL_GLContext * poglContext);
static void quit(void);
static void initGL(SDL_Window * win);
static void initData(void);
static void resizeGL(SDL_Window * win);
static void loop(SDL_Window * win);
static void manageEvents(SDL_Window * win);
static void draw(void);
static void printFPS(void);

/*!\brief pointeur vers la (future) fenêtre SDL */
static SDL_Window * _win = NULL;
/*!\brief pointeur vers le (futur) contexte OpenGL */
static SDL_GLContext _oglContext = NULL;
/*!\brief identifiant du (futur) vertex array object */
static GLuint _vao[6] ={0,0,0,0,0,0};
//static GLuint _vao2= 0;
/*!\brief identifiant du (futur) buffer de data */
static GLuint _buffer[6] ={0,0,0,0,0,0};
//static GLuint _buffer2 = 0;
/*!\brief identifiant du (futur) GLSL program */
static GLuint _pId = 0;
static GLuint _pId2= 0;
static GLuint _rId= 0;
/*!\brief identifiant de la texture */
/*Mario
  0:Walk1
  1:Walk2
  2:Walk3
  3:Slide
  4:Stand
  5:Jump
  6:Sit 1
  7:Sit 2
  8:Sitbig
  9:swim1
  10:swim2
  11:swim3
  12:swim4
  13:swim5
  14:dead
*/
static GLuint _tId[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //mario
static GLuint _tId2 = 0; // Level
static GLuint _tIdb[2]= {0,0}; // Block
static GLuint _tIdm= 0; // Mushroom
static GLuint _tIdt[2]= {0,0}; // Title
static GLuint _tIdland[7]= {0,0,0,0,0,0,0}; // Land

/*!\brief Structure parametre */

typedef struct param{
  float vx;
  float vy;
  float vz;
  float vsc;
  float vred;
  float vgreen;
  float vblue;
  float valpha;
  float currx;
  float curry;
  float currz;
  float currsc;
  float currred;
  float currgreen;
  float currblue;
  float curralpha;
} param;

static Mix_Music * _mmusic = NULL;
/*!\brief La fonction principale initialise la bibliothèque SDL2,
 * demande la création de la fenêtre SDL et du contexte OpenGL par
 * l'appel à \ref initWindow, initialise OpenGL avec \ref initGL et
 * lance la boucle (infinie) principale.
 */
static void initAudio(char * filename) {
  int mixFlags = MIX_INIT_MP3 | MIX_INIT_OGG, res;
  res = Mix_Init(mixFlags);
  if( (res & mixFlags) != mixFlags ) {
    fprintf(stderr, "Mix_Init: Erreur lors de l'initialisation de la bibliothèque SDL_Mixer\n");
    fprintf(stderr, "Mix_Init: %s\n", Mix_GetError());
    //   exit(-3);
  }
  if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 1024) < 0)
    exit(-4);  
  if(!(_mmusic = Mix_LoadMUS(filename))) {
    fprintf(stderr, "Erreur lors du Mix_LoadMUS: %s\n", Mix_GetError());
    exit(-5);
  }
}


int main(int argc, char ** argv) {
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Erreur lors de l'initialisation de SDL :  %s", SDL_GetError());
    return -1;
  }
  atexit(SDL_Quit);
  initAudio("soundtrack.mp3");
  if((_win = initWindow(800, 600, &_oglContext))) {
    initGL(_win);
    _pId = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
    _pId2 = gl4duCreateProgram("<vs>shaders/basic2.vs", "<fs>shaders/basic2.fs", NULL);
    _rId = gl4duCreateProgram("<vs>shaders/rotate.vs", "<fs>shaders/basic.fs", NULL);
    initData();
    loop(_win);
  }
  return 0;
}

/*!\brief Cette fonction est appelée au moment de sortir du programme
 *  (atexit), elle libère la fenêtre SDL \ref _win et le contexte
 *  OpenGL \ref _oglContext.
 */
static void quit(void) {
  if(_mmusic) {
    while(Mix_PlayingMusic() && Mix_FadeOutMusic(1000)) {
      draw();
      SDL_GL_SwapWindow(_win);
      SDL_Delay(1);
    }
    Mix_FreeMusic(_mmusic);
    _mmusic = NULL;
  }
  Mix_CloseAudio();
  Mix_Quit();
  if(_vao)
    glDeleteVertexArrays(6, _vao);
  if(_buffer)
    glDeleteBuffers(6, _buffer);
  if(_buffer){
    glDeleteTextures(15, _tId);
    glDeleteTextures(1, &_tId2);
    glDeleteTextures(2, _tIdb);
    glDeleteTextures(1, &_tIdm);
    glDeleteTextures(2, _tIdt);
    glDeleteTextures(7, _tIdland);
  }
  if(_oglContext)
    SDL_GL_DeleteContext(_oglContext);
  if(_win)
    SDL_DestroyWindow(_win);
  gl4duClean(GL4DU_ALL);
}

/*!\brief Cette fonction créé la fenêtre SDL de largeur \a w et de
 *  hauteur \a h, le contexte OpenGL \ref et stocke le pointeur dans
 *  poglContext. Elle retourne le pointeur vers la fenêtre SDL.
 *
 * Le contexte OpenGL créé est en version 3 pour
 * SDL_GL_CONTEXT_MAJOR_VERSION, en version 2 pour
 * SDL_GL_CONTEXT_MINOR_VERSION et en SDL_GL_CONTEXT_PROFILE_CORE
 * concernant le profile. Le double buffer est activé et le buffer de
 * profondeur est en 24 bits.
 *
 * \param w la largeur de la fenêtre à créer.
 * \param h la hauteur de la fenêtre à créer.
 * \param poglContext le pointeur vers la case où sera référencé le
 * contexte OpenGL créé.
 * \return le pointeur vers la fenêtre SDL si tout se passe comme
 * prévu, NULL sinon.
 */
static SDL_Window * initWindow(int w, int h, SDL_GLContext * poglContext) {
  SDL_Window * win = NULL;
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  if( (win = SDL_CreateWindow("Fenetre GL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			      800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | 
			      SDL_WINDOW_SHOWN)) == NULL )
    return NULL;
  if( (*poglContext = SDL_GL_CreateContext(win)) == NULL ) {
    SDL_DestroyWindow(win);
    return NULL;
  }
  fprintf(stderr, "Version d'OpenGL : %s\n", glGetString(GL_VERSION));
  fprintf(stderr, "Version de shaders supportes : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));  
  atexit(quit);
  return win;
}

/*!\brief Cette fonction initialise les paramètres OpenGL.
 *
 * \param win le pointeur vers la fenêtre SDL pour laquelle nous avons
 * attaché le contexte OpenGL.
 */
static void initGL(SDL_Window * win) {
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.2f, 0.2f, 0.9f, 0.0f);
  resizeGL(win);
}

static void initData(void) {
  SDL_Surface * t=NULL;
  GLfloat mario[] = {
    /* 4 coordonnées de sommets */
    -0.08f, -0.15f, -0.8f,
    0.08f, -0.15f, -0.8f,
    -0.08f,  0.15f,-0.8f,
    0.08f,  0.15f,-0.8f,
    /* 2 coordonnées de texture par sommet */
    0.0f, 0.0f,
    1.0f, 0.0f, 
    0.0f, 1.0f,
    1.0f, 1.0f
  };


  GLfloat title[] = {
    /* 4 coordonnées de sommets */
    -1.0f, -1.0f, -0.6f,
    1.0f, -1.0f,-0.6f,
    -1.0f,  1.0f,-0.6f,
    1.0f,  1.0f,-0.6f,
    /* 2 coordonnées de texture par sommet */
    0.0f, 0.0f,
    1.0f, 0.0f, 
    0.0f, 1.0f,
    1.0f, 1.0f
  };
  

  GLfloat level[] = {
    /* 4 coordonnées de sommets */
    -1.0f, -1.0f, 0.9f,
    1.0f, -1.0f, 0.9f,
    -1.0f,  1.0f, 0.9f,
    1.0f,  1.0f, 0.9f,
    /* 2 coordonnées de texture par sommet */
    0.0f, 0.0f,
    1.0f, 0.0f, 
    0.0f, 1.0f,
    1.0f, 1.0f
  };
  
  GLfloat intblock[]= {
    /* 4 coordonnées de sommets */
    -0.1f, 0.0f, -0.8f,
    0.1f, 0.0f, -0.8f,
    -0.1f, 0.2f,-0.8f,
    0.1f, 0.2f,-0.8f,
    /* 2 coordonnées de texture par sommet */
    0.0f, 0.0f,
    1.0f, 0.0f, 
    0.0f, 1.0f,
    1.0f, 1.0f
  };

  GLfloat mush[]= {
    /* 4 coordonnées de sommets */
    -0.1f, 0.0f, -0.8f,
    0.1f, 0.0f, -0.8f,
    -0.1f, 0.2f,-0.8f,
    0.1f, 0.2f,-0.8f,
    /* 2 coordonnées de texture par sommet */
    0.0f, 0.0f,
    1.0f, 0.0f, 
    0.0f, 1.0f,
    1.0f, 1.0f
  };

  GLfloat land[] = {
    /* 4 coordonnées de sommets */
    -1.0f, -1.0f, 0.9f,
    1.0f, -1.0f, 0.9f,
    -1.0f,  1.0f, 0.9f,
    1.0f,  1.0f, 0.9f,
    /* 2 coordonnées de texture par sommet */
    0.0f, 0.0f,
    1.0f, 0.0f, 
    0.0f, 1.0f,
    1.0f, 1.0f
  };

  //Mario
  glGenVertexArrays(6, _vao);
  glBindVertexArray(_vao[0]);
  //printf("vao 0 = %d\n",_vao[0]);
  glGenTextures(15, _tId);
  
  glGenBuffers(6, _buffer);
  //printf("buffer 0 = %d\n",_buffer[0]);
  //printf("buffer 1 = %d\n",_buffer[1]);
  //printf("buffer 2 = %d\n",_buffer[2]);
  //printf("buffer 3 = %d\n",_buffer[3]);
  glBindBuffer(GL_ARRAY_BUFFER, _buffer[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof mario, mario, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);  
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)((4 * 3) * sizeof *mario));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindTexture(GL_TEXTURE_2D, _tId[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_walk1.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(walk1)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_walk2.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(walk2)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[2]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_walk3.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(walk3)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }


  glBindTexture(GL_TEXTURE_2D, _tId[3]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_slide.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(slide)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[4]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_stand.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(stand)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[5]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_jump.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(jump)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[6]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_sit1.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(sit1)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[7]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_sit2.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(sit2)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[8]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_sitbig.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(sitbig)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[9]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_swim1.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(swim1)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[10]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_swim2.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(swim2)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[11]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_swim3.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(swim3)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[12]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_swim4.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(swim4)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[13]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_swim5.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(swim5)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tId[14]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_dead.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(swim5)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);

  //level
  glBindVertexArray(_vao[1]);
  //printf("vao 1 = %d\n",_vao[1]);
  glGenTextures(1, &_tId2);
  glBindBuffer(GL_ARRAY_BUFFER, _buffer[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof level, level, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)((4 * 3) * sizeof *level));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, _tId2);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_Mario_level.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);

  //Block
  glBindVertexArray(_vao[2]);
  //printf("vao 2 = %d\n",_vao[2]);
  glGenTextures(2, _tIdb);
  glBindBuffer(GL_ARRAY_BUFFER, _buffer[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof intblock, intblock, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)((4 * 3) * sizeof *intblock));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
 
  glBindTexture(GL_TEXTURE_2D, _tIdb[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_intb1.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture (intb)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, _tIdb[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_intb2.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(intb2)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);

  //Mushroom
  glBindVertexArray(_vao[3]);
  glGenTextures(1, &_tIdm);
  glBindBuffer(GL_ARRAY_BUFFER, _buffer[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof mush, mush, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)((4 * 3) * sizeof *mush));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, _tIdm);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("8_Bit_mush.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
    
  //title
  glBindVertexArray(_vao[4]);
  //printf("vao 1 = %d\n",_vao[4]);
  glGenTextures(2, _tIdt);
  glBindBuffer(GL_ARRAY_BUFFER, _buffer[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof title, title, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)((4 * 3) * sizeof *title));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, _tIdt[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("title.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  

  glBindTexture(GL_TEXTURE_2D, _tIdt[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Over.png")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  
    
  //Land
  glBindVertexArray(_vao[5]);
  //printf("vao 6 = %d\n",_vao[5]);
  glGenTextures(7, _tIdland);
  glBindBuffer(GL_ARRAY_BUFFER, _buffer[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof land, land, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const void *)((4 * 3) * sizeof *land));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, _tIdland[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Machu_Picchu.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, _tIdland[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Himalaya.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, _tIdland[2]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Clouds.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, _tIdland[3]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Earth.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, _tIdland[4]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("asteroid_field.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, _tIdland[5]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Galaxy.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, _tIdland[6]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  if( (t = IMG_Load("Black_Hole.jpg")) != NULL ) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->w, t->h, 0, GL_RGB, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "Erreur lors du chargement de la texture(level)\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
  
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}


/*!\brief Cette fonction paramétrela vue (viewPort) OpenGL en fonction
 * des dimensions de la fenêtre SDL pointée par \a win.
 *
 * \param win le pointeur vers la fenêtre SDL pour laquelle nous avons
 q * attaché le contexte OpenGL.
*/
static void resizeGL(SDL_Window * win) {
  int w, h;
  SDL_GetWindowSize(win, &w, &h);
  glViewport(0, 0, w, h);
}

/*!\brief Boucle infinie principale : gère les évènements, dessine,
 * imprime le FPS et swap les buffers.
 *
 * \param win le pointeur vers la fenêtre SDL pour laquelle nous avons
 * attaché le contexte OpenGL.
 */
static void loop(SDL_Window * win) {
  SDL_GL_SetSwapInterval(1);
  for(;;) {
    manageEvents(win);
    draw();
    // printFPS();
    SDL_GL_SwapWindow(win);
    gl4duUpdateShaders();
    if(!Mix_PlayingMusic())
    Mix_PlayMusic(_mmusic, -1);
  }
}

/*!\brief Cette fonction permet de gérer les évènements clavier et
 * souris via la bibliothèque SDL et pour la fenêtre pointée par \a
 * win.
 *
 * \param win le pointeur vers la fenêtre SDL pour laquelle nous avons
 * attaché le contexte OpenGL.
 */

static void manageEvents(SDL_Window * win) {
  SDL_Event event;
  while(SDL_PollEvent(&event)) 
    switch (event.type) {
    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
      case SDLK_ESCAPE:
      case 'q':
	exit(0);
      default:
	fprintf(stderr, "La touche %s a ete pressee\n",
		SDL_GetKeyName(event.key.keysym.sym));
	break;
      }
      break;
    case SDL_KEYUP:
      break;
    case SDL_WINDOWEVENT:
      if(event.window.windowID == SDL_GetWindowID(win)) {
	switch (event.window.event)  {
	case SDL_WINDOWEVENT_RESIZED:
	  resizeGL(win);
	  break;
	case SDL_WINDOWEVENT_CLOSE:
	  event.type = SDL_QUIT;
	  SDL_PushEvent(&event);
	  break;
	}
      }
      break;
    case SDL_QUIT:
      exit(0);
    }
}

/*!\brief Cette fonction dessine dans le contexte OpenGL actif.
 */

static int intro(void){
  static param ptitle ={/* vx */ 0.0,/* vy */ 0.0,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ 0.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static int frame=0;


  if(frame>200){
    ptitle.vy=0.005;
    ptitle.curry+=ptitle.vy;
    if(ptitle.curry >= 0.8)
      return 1;
  }
  glUseProgram(_pId);
  glBindVertexArray(_vao[4]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tIdt[0]);
  glUniform1f(glGetUniformLocation(_pId, "cx"), ptitle.currx);
  glUniform1f(glGetUniformLocation(_pId, "cy"), ptitle.curry);
  glUniform1f(glGetUniformLocation(_pId, "cz"), ptitle.currz);
  glUniform1f(glGetUniformLocation(_pId, "cscale"), ptitle.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  frame++;
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);
  return 0;
}

static int walk_mario(void){
  static param pmario ={/* vx */ 0.015,/* vy */ 0.0,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ -1.0, /* curry */ -0.54, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static param pblock ={/* vx */ 0.0,/* vy */ 0.0,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ 0.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static GLuint frame= 0;
  static GLuint once=0;
  if(once==1 && pmario.currx >=0)
    return 2;
  if(pmario.currx>= 1.0){
    pmario.currx=-1.0;
    once=1;
  }
  pmario.currx += pmario.vx;
  pmario.curry += pmario.vy;
  glUseProgram(_pId);
  glBindVertexArray(_vao[0]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  if(once == 1 && pmario.currx>= -0.25){
    glBindTexture(GL_TEXTURE_2D, _tId[3]);
    pmario.vx=0.005;
  }
  else
    glBindTexture(GL_TEXTURE_2D, _tId[(frame/10)%3]);
  // printf("cx= %f\n",pmario.currx);
  glUniform1f(glGetUniformLocation(_pId, "cx"), pmario.currx);
  //printf("cx= %f\n",pmario.curry);
  glUniform1f(glGetUniformLocation(_pId, "cy"), pmario.curry);
  //printf("cx= %f\n",pmario.currz);
  glUniform1f(glGetUniformLocation(_pId, "cz"), pmario.currz);
  //printf("cx= %f\n",pmario.currsc);
  //printf("frame= %d\n",frame);
  frame++;
  glUniform1f(glGetUniformLocation(_pId, "cscale"), pmario.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);
  //IntBlock
  if(once){
    glUseProgram(_pId);
    glBindVertexArray(_vao[2]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tIdb[0]);
    glUniform1f(glGetUniformLocation(_pId, "cx"), pblock.currx);
    glUniform1f(glGetUniformLocation(_pId, "cy"), pblock.curry);
    glUniform1f(glGetUniformLocation(_pId, "cz"), pblock.currz);
    glUniform1f(glGetUniformLocation(_pId, "cscale"), pblock.currsc);
    glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
    glUseProgram(0);
  }
  return 1;
}

static int jump_mario(void){
  static param pmario ={/* vx */ 0.0,/* vy */ 0.02,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ -0.54, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static param pblock ={/* vx */ 0.0,/* vy */ 0.0,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ 0.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static GLuint frame= 0;
     
  glUseProgram( _pId );
  glBindVertexArray(_vao[0]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  
  if(frame >=100 && pmario.curry <= -0.15){
    pmario.curry += pmario.vy;
    glBindTexture(GL_TEXTURE_2D, _tId[5]);
    if(pmario.curry >=-0.15){
      return 3;
    }
  }else
    glBindTexture(GL_TEXTURE_2D, _tId[4]);
  // printf("cx= %f\n",pmario.currx);
  glUniform1f(glGetUniformLocation(_pId, "cx"), pmario.currx);
  //printf("cx= %f\n",pmario.curry);
  glUniform1f(glGetUniformLocation(_pId, "cy"), pmario.curry);
  //printf("cx= %f\n",pmario.currz);
  glUniform1f(glGetUniformLocation(_pId, "cz"), pmario.currz);
  //printf("cx= %f\n",pmario.currsc);
  //printf("frame= %d\n",frame);
  frame++;
  glUniform1f(glGetUniformLocation(_pId, "cscale"), pmario.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);
  //Block
  glUseProgram(_pId);
  glBindVertexArray(_vao[2]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tIdb[0]);
  glUniform1f(glGetUniformLocation(_pId, "cx"), pblock.currx);
  glUniform1f(glGetUniformLocation(_pId, "cy"), pblock.curry);
  glUniform1f(glGetUniformLocation(_pId, "cz"), pblock.currz);
  glUniform1f(glGetUniformLocation(_pId, "cscale"), pblock.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);
  return 2;
}

static int block_hit(void){
  static param pmario ={/* vx */ 0.0,/* vy */ -0.002,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ -0.15, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static param pblock ={/* vx */ 0.0,/* vy */ 0.01,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ 0.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static param pmush ={/* vx */ 0.0,/* vy */ 0.001,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ 0.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};

  static GLuint frame= 0;
  static GLuint once= 0;
  glUseProgram( _pId );
  glBindVertexArray(_vao[0]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  
  if(pmario.curry >= -0.54){
    pmario.vy+= -0.001;
    pmario.curry += pmario.vy;
    glBindTexture(GL_TEXTURE_2D, _tId[6]);
  }else if(frame <= 100)
    glBindTexture(GL_TEXTURE_2D, _tId[7]);
  else
    glBindTexture(GL_TEXTURE_2D, _tId[4]);
  // printf("cx= %f\n",pmario.currx);
  glUniform1f(glGetUniformLocation(_pId, "cx"), pmario.currx);
  //printf("cx= %f\n",pmario.curry);
  glUniform1f(glGetUniformLocation(_pId, "cy"), pmario.curry);
  //printf("cx= %f\n",pmario.currz);
  glUniform1f(glGetUniformLocation(_pId, "cz"), pmario.currz);
  //printf("cx= %f\n",pmario.currsc);
  //printf("frame= %d\n",frame);
  frame++;
  glUniform1f(glGetUniformLocation(_pId, "cscale"), pmario.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);
  //Block
  glUseProgram(_pId);
  glBindVertexArray(_vao[2]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tIdb[1]);
  if(pblock.curry <= 0.1 && once==0){
    pblock.vy=0.01;
    pblock.curry+= pblock.vy;
  }else if(pblock.curry >=0.0){
    once=1;
    pblock.vy= -0.01;
    pblock.curry+=pblock.vy;
  }
  else{
    pblock.vy=0;
    once=-1;
  } 
  glUniform1f(glGetUniformLocation(_pId, "cx"), pblock.currx);
  glUniform1f(glGetUniformLocation(_pId, "cy"), pblock.curry);
  glUniform1f(glGetUniformLocation(_pId, "cz"), pblock.currz);
  glUniform1f(glGetUniformLocation(_pId, "cscale"), pblock.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);

  if(once== -1 && pmush.curry >= pmario.curry){
    static int out=0;
    glUseProgram(_pId);
    glBindVertexArray(_vao[3]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tIdm);
    if(out == 0){
      pmush.vy=0.002;
      pmush.curry+=pmush.vy;
      if(pmush.curry>=0.16){
	out=1;
	pmush.curry=0.2;
      }
    }
    if(out == 1){
      pmush.vy-= 0.0001;
      pmush.curry+=pmush.vy;
    }
    glUniform1f(glGetUniformLocation(_pId, "cx"), pmush.currx);
    glUniform1f(glGetUniformLocation(_pId, "cy"), pmush.curry);
    glUniform1f(glGetUniformLocation(_pId, "cz"), pmush.currz);
    glUniform1f(glGetUniformLocation(_pId, "cscale"), pmush.currsc);
    glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
    glUseProgram(0);
  }    
  if(pmario.curry >=pmush.curry){
    return 4;
  }
  else
    return 3;
}

static int mush_get(void){
  static param pmario ={/* vx */ 0.013,/* vy */ 0.013,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ -0.54, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static param pblock ={/* vx */ 0.0,/* vy */ 0.0,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ 0.0, /* curry */ 0.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static GLuint frame= 0;
  static int done=0;
  static float teta=0;
  //static GLuint once= 0;
  //Mario 
  if(done == 0){
    glUseProgram( _pId );
    glBindVertexArray(_vao[0]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    if(frame<=150)
      glBindTexture(GL_TEXTURE_2D, _tId[(frame)%9]);
    else
      glBindTexture(GL_TEXTURE_2D, _tId[(frame/5)%5 + 9]);
    glUniform1f(glGetUniformLocation(_pId, "cx"), pmario.currx);
    glUniform1f(glGetUniformLocation(_pId, "cy"), pmario.curry);
    glUniform1f(glGetUniformLocation(_pId, "cz"), pmario.currz);
    //printf("frame= %d\n",frame);
    frame++;
    glUniform1f(glGetUniformLocation(_pId, "cscale"), pmario.currsc);
    glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
    glUseProgram(0);  
    if(frame >= 200)
      done = 1;
  }
  //BLAST OFF
  if(done ==1){
    glUseProgram( _rId );
    glBindVertexArray(_vao[0]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tId[(frame/5)%5 + 9]);
    pmario.currx+=pmario.vx;
    pmario.curry+=pmario.vy;
    //printf("currx= %f\n", pmario.currx);
    //printf("curry= %f\n", pmario.curry);
    glUniform1f(glGetUniformLocation(_rId, "cx"), pmario.currx);
    glUniform1f(glGetUniformLocation(_rId, "cy"), pmario.curry);
    glUniform1f(glGetUniformLocation(_rId, "cz"), pmario.currz);
    glUniform1f(glGetUniformLocation(_rId, "teta"),teta+=0.2);
    //printf("frame= %d\n",frame);
    frame++;
    glUniform1f(glGetUniformLocation(_rId, "cscale"), pmario.currsc);
    glUniform1i(glGetUniformLocation(_rId, "myTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
    glUseProgram(0);      
  }
    //Block
    glUseProgram(_pId);
    glBindVertexArray(_vao[2]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tIdb[1]); 
    glUniform1f(glGetUniformLocation(_pId, "cx"), pblock.currx);
    glUniform1f(glGetUniformLocation(_pId, "cy"), pblock.curry);
    glUniform1f(glGetUniformLocation(_pId, "cz"), pblock.currz);
    glUniform1f(glGetUniformLocation(_pId, "cscale"), pblock.currsc);
    glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
    glUseProgram(0);
    if(pmario.currx >= 1.0)
      return 5;
  return 4;
}

static void level(void){
  glUseProgram(_pId);
  glBindVertexArray(_vao[1]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tId2);
  glUniform1f(glGetUniformLocation(_pId, "cx"), 0);
  glUniform1f(glGetUniformLocation(_pId, "cy"), 0); 
  glUniform1f(glGetUniformLocation(_pId, "cz"),  0);
  glUniform1f(glGetUniformLocation(_pId, "cscale"), 0);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glUseProgram(0);
}

static int trip(void){
  static param pmario ={/* vx */ 0.017,/* vy */ 0.017,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ -2.0, /* curry */ -1.8, /* currz */ 0.0, /* currsc */1.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static param ptitle ={/* vx */ 0.0,/* vy */ -0.003,/* vz */ 0.0,/* vsc */ 0.0,/* vred */ 0.0,/* vgreen */ 0.0,/* vblue */ 0.0,/* valpha */ 0.0,/* currx */ -0.0, /* curry */ 1.0, /* currz */ 0.0, /* currsc */0.0,/* currred */0.0, /* currgreen */ 0.0, /* currblue */0.0,/* curralpha */ 0.0};
  static GLuint frame= 0;
  static float teta=0;
  static int done=0;
  glUseProgram(_pId);
  glBindVertexArray(_vao[5]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  if(frame <=230){
    glBindTexture(GL_TEXTURE_2D, _tIdland[0]);
    if(frame == 230){
      pmario.curry= -2.0;
      pmario.currx= -1.0;
    }
  }
  if(frame >230 && frame <=460){
    glBindTexture(GL_TEXTURE_2D, _tIdland[1]);
    pmario.vy=0.018;
    pmario.vx=0;
    if(frame == 460){
      pmario.curry= -2.0;
      pmario.currx= -0.0;
    }
  }
  if(frame >460 && frame <=690){
    glBindTexture(GL_TEXTURE_2D, _tIdland[2]);
    pmario.vy=0.018;
    pmario.vx=0;
    pmario.vsc=0.02;
    if(frame == 690){
      pmario.curry= 0.3;
      pmario.currx= 1.3;
      pmario.currsc=3.0;
    }
  }
  if(frame >690 && frame <=940){
    glBindTexture(GL_TEXTURE_2D, _tIdland[3]);
    pmario.vx=0.012;
    pmario.vy=0.012;
    pmario.vsc=0.0;
    if(frame == 940){
      pmario.curry= 0.4;
      pmario.currx= 0.3;
      pmario.currsc=0.01;
    }
  }
  if(frame >940 && frame <=1190){
    glBindTexture(GL_TEXTURE_2D, _tIdland[4]);
    pmario.vx=0.0;
    pmario.vy=0.0;
    pmario.vsc=0.030;
    if(frame == 1190){
      pmario.curry= -3.5;
      pmario.currx= 3.5;
      pmario.currsc=3.0;
    }
  }
  if(frame >1190 && done == 0){
    if(frame >= 1420 && pmario.currx<=4){
	glBindTexture(GL_TEXTURE_2D, _tIdland[5]);
	pmario.vx=0.01;
	pmario.vy=0.0;
	pmario.vsc=0.0;
    }
    else if(frame<1420){
      glBindTexture(GL_TEXTURE_2D, _tIdland[5]);
      pmario.vx=-0.01;
      pmario.vy=0.01;
      pmario.vsc=0.0;
    }
    else{
      done = 1;
      pmario.currx=-2.0;
      pmario.curry= 0.0;
      pmario.currsc=1.0;
    }
  }
  if(done == 1){
      glBindTexture(GL_TEXTURE_2D, _tIdland[6]);
      if(pmario.currx<0)
	pmario.vx=0.005;
      else
	pmario.vx=0;
      pmario.vsc=0.02;
      pmario.vy=0.0;
  }

  glUniform1f(glGetUniformLocation(_pId, "cx"), 0);
  glUniform1f(glGetUniformLocation(_pId, "cy"), 0); 
  glUniform1f(glGetUniformLocation(_pId, "cz"),  0);
  glUniform1f(glGetUniformLocation(_pId, "cscale"), 0);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  frame++;
  //printf("frame: %d\n",frame);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glUseProgram(0);
  //Mario
  glUseProgram( _rId );
  glBindVertexArray(_vao[0]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  if(frame<1420)
    glBindTexture(GL_TEXTURE_2D, _tId[(frame/5)%5 + 9]);
  else
    glBindTexture(GL_TEXTURE_2D, _tId[14]);
  pmario.currx+=pmario.vx;
  pmario.curry+=pmario.vy;
  pmario.currsc+=pmario.vsc;
  glUniform1f(glGetUniformLocation(_rId, "cx"), pmario.currx);
  glUniform1f(glGetUniformLocation(_rId, "cy"), pmario.curry);
  glUniform1f(glGetUniformLocation(_rId, "cz"), pmario.currz);
  if(frame<1420)
    glUniform1f(glGetUniformLocation(_rId, "teta"),teta+=0.5);
  else
    glUniform1f(glGetUniformLocation(_rId, "teta"),teta+=0.3);
  // printf("frame= %d\n",frame);
  //frame++;
  //printf("scale:%f\n", pmario.currsc);
  glUniform1f(glGetUniformLocation(_rId, "cscale"), pmario.currsc);
  glUniform1i(glGetUniformLocation(_rId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);      
  //printf("currx: %f\n",pmario.currx); 
  if(pmario.currx>=0.0 && frame >= 1700){
  
  glUseProgram(_pId);
  glBindVertexArray(_vao[4]);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tIdt[1]);
  //printf("currytitle= %f\n",ptitle.curry);
  if(ptitle.curry>=0.0)
    ptitle.curry+=ptitle.vy;
  if(ptitle.curry <=0.0 && frame >2400){
    quit();
    exit(-1);
  }
  glUniform1f(glGetUniformLocation(_pId, "cx"), ptitle.currx);
  glUniform1f(glGetUniformLocation(_pId, "cy"), ptitle.curry);
  glUniform1f(glGetUniformLocation(_pId, "cz"), ptitle.currz);
  glUniform1f(glGetUniformLocation(_pId, "cscale"), ptitle.currsc);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindVertexArray(0);
  glUseProgram(0);  
  }  
  return 5;
}

static void draw(void) {
  static int who=0;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //printf("who == %d\n",who);
  if(who==0)
    who=intro();
  if(who==1)
    who=walk_mario();
  if(who==2)
    who=jump_mario();
  if(who==3)
    who=block_hit();
  if(who==4)
    who=mush_get();
  if(who<5)
    level();
  if(who == 5)
    who=trip();
}

/*!\brief Cette fonction imprime le FPS (Frames Per Second) de
 * l'application.
 */
/* static void printFPS(void) { */
/*   Uint32 t; */
/*   static Uint32 t0 = 0, f = 0; */
/*   f++; */
/*   t = SDL_GetTicks(); */
/*   if(t - t0 > 2000) { */
/*     fprintf(stderr, "%8.2f\n", (1000.0 * f / (t - t0))); */
/*     t0 = t; */
/*     f  = 0; */
/*   } */
/* } */

