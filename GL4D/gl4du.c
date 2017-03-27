/*!\file gl4du.c
 *
 * \brief The GL4Dummies Utilities
 *
 * \author Farès BELHADJ amsi@grafeet.fr (ou amsi@ai.univ-paris8.fr)
 * \date March 28, 2008
 */

#include <gl4du.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>
#include <aes.h>
#if !defined(_MSC_VER)
#include <errno.h>
#include <limits.h>
#endif

typedef struct shader_t shader_t;
typedef struct program_t program_t;

struct shader_t {
  GLuint id;
  GLenum shadertype;
  char * filename;
  time_t mod_time;
  unsigned todelete:1;
  int nprograms, sprograms;
  program_t ** programs;
  shader_t * next;
};

struct program_t {
  GLuint id;
  int nshaders, sshaders;
  shader_t ** shaders;
  program_t * next;
};

/*!\brief liste de vertex et fragment shaders. Chaque shader est
 * composé d'un id (GL), du type, du nom de fichier et de la date de
 * modification du fichier.
 */
static shader_t * shaders_list = NULL;

/*!\brief liste de programs. Chaque program est composé d'un id (GL),
 * il est lié à une liste de shaders.
 */
static program_t * programs_list = NULL;

static shader_t **  findfnInShadersList(char * filename);
static shader_t **  findidInShadersList(GLuint id);
static shader_t **  addInShadersList(GLenum shadertype, char * filename);
static shader_t **  addInShadersListFED(char * decData, GLenum shadertype, char * filename);
static void         deleteFromShadersList(shader_t ** shp);
static program_t ** findInProgramsList(GLuint id);
static program_t ** addInProgramsList(GLuint id);
static void         deleteFromProgramsList(program_t ** pp);
static void         attachShader(program_t * prg, shader_t * sh);
static void         detachShader(program_t * prg, shader_t * sh);

/*!\brief imprime s'il existe l'infoLog de la compilation du Shader
 * \a object dans \a fp.
 */
void gl4duPrintShaderInfoLog(GLuint object, FILE * fp) {
  char * log = NULL;
  int maxLen = 0, len = 0;
  glGetShaderiv(object, GL_INFO_LOG_LENGTH, &maxLen);
  if(maxLen > 1) {
    log = malloc(maxLen * sizeof * log);
    assert(log);
    glGetShaderInfoLog(object, maxLen, &len, log);
    fprintf(fp, "%s (%d): %s (ID = %d)\n%s\n", __FILE__, __LINE__, __func__, object, log);
    free(log);
  }
}

/*!\brief imprime s'il existe l'infoLog de l'édition de liens pour le
 * Program \a object dans \a fp.
 */
void gl4duPrintProgramInfoLog(GLuint object, FILE * fp) {
  char * log = NULL;
  int maxLen = 0, len = 0;
  glGetProgramiv(object, GL_INFO_LOG_LENGTH, &maxLen);
  if(maxLen > 1) {
    log = malloc(maxLen * sizeof * log);
    assert(log);
    glGetProgramInfoLog(object, maxLen, &len, log);
    fprintf(fp, "%s (%d): %s (ID = %d)\n%s\n", __FILE__, __LINE__, __func__, object, log);
    free(log);
  }
}

/*!\brief imprime dans le fichier pointé par \a fp le
 * Frame-Per-Second.
 *
 * Cette fonction doit être appelée dans la loop.
 */
void gl4duPrintFPS(FILE * fp) {
  double t;
  static double t0 = 0;
  static unsigned int f = 0;
  f++;
  t = gl4dGetElapsedTime();
  if(t - t0 > 2000.0) {
	  fprintf(fp, "%8.2f\n", (1000.0 * f / (t - t0)));
      t0 = t;
      f  = 0;
  }
}

/*!\brief retourne l'identifiant du shader décrit dans \a filename.
 *
 * Soit le shader existe déjà et l'identifiant est juste retourné avec
 * \ref findfnInShadersList, soit il est créer en utilisant \ref
 * gl4dCreateShader, \ref gl4dShaderSource et \ref
 * gl4dCompileShader. Il se peut que le shader soit mis à jour si la
 * date de modification du fichier est differente de celle stoquée
 * dans \shaders_list. Dans ce cas le shader est détruit puis recréé
 * et la modification doit être répercuté sur tous les PROGRAMs liés.
 *
 * \ref l'identifiant du shader décrit dans \a filename.
 */
GLuint gl4duCreateShader(GLenum shadertype, char * filename) {
  shader_t ** sh = findfnInShadersList(filename);
  if(*sh) return (*sh)->id;
  sh = addInShadersList(shadertype, filename);
  return (sh) ? (*sh)->id : 0;
}

/*!\brief retourne l'identifiant du shader décrit dans \a filename.
 * Version FED de la précédente
 * \todo commenter
 */
GLuint gl4duCreateShaderFED(char * decData, GLenum shadertype, char * filename) {
  shader_t ** sh = findfnInShadersList(filename);
  if(*sh) return (*sh)->id;
  sh = addInShadersListFED(decData, shadertype, filename);
  return (sh) ? (*sh)->id : 0;
}

/*!\brief retourne l'identifiant du shader décrit dans \a filename.
 *
 * Soit le shader existe et l'identifiant est retourné soit la
 * fonction retourne 0.
 *
 * \ref l'identifiant du shader décrit dans \a filename ou 0 s'il n'a
 * pas été chargé.
 */
GLuint gl4duFindShader(char * filename) {
  shader_t ** sh = findfnInShadersList(filename);
  return (*sh) ? (*sh)->id : 0;
}

/*!\brief supprime le shader dont l'identifiant openGL est \a id de la
 * liste de shaders \ref shaders_list.
 */
void gl4duDeleteShader(GLuint id) {
  shader_t ** sh = findidInShadersList(id);
  if(*sh) {
    if((*sh)->nprograms > 0) 
      (*sh)->todelete = 1;
    else
      deleteFromShadersList(sh);
  }
}

/*!\brief créé un program à partir d'une liste (variable) de nom de
 * fichiers shaders et renvoie l'identifiant openGL du program créé.
 *
 * Les arguments sont les noms des fichiers shaders précédés d'un tag
 * indiquant le type de shader : <vs> pour un vertex shader et <fs>
 * pour un fragment shader et <gs> pour un geometry shader.
 *
 * \return l'identifiant openGL du program créé sinon 0. Dans ce
 * dernier cas les shaders créés (compilés) avant l'échec ne sont pas
 * supprimés ; un appel à \ref gl4duCleanUnattached peut s'en charger.
 */
GLuint gl4duCreateProgram(char * firstone, ...) {
  va_list  pa;
  char * filename;
  program_t ** prg;
  GLuint sId, pId = glCreateProgram();
  if(!pId) return pId;
  prg = addInProgramsList(pId);

  filename = firstone;
  va_start(pa, firstone);
  fprintf(stderr, "%s (%d): Creation du programme %d a l'aide des Shaders :\n", __FILE__, __LINE__, pId);
  do {
    if(!strncmp("<vs>", filename, 4)) { /* vertex shader */
      if(!(sId = gl4duCreateShader(GL_VERTEX_SHADER, &filename[4]))) goto gl4duCreateProgram_ERROR;
      attachShader(*prg, *findidInShadersList(sId));
      fprintf(stderr, "\t\t%s : vertex shader\n", &filename[4]);
    } else if(!strncmp("<fs>", filename, 4)) { /* fragment shader */
      if(!(sId = gl4duCreateShader(GL_FRAGMENT_SHADER, &filename[4]))) goto gl4duCreateProgram_ERROR;
      attachShader(*prg, *findidInShadersList(sId));
      fprintf(stderr, "\t\t%s : fragment shader\n", &filename[4]);
    } else if(!strncmp("<gs>", filename, 4)) { /* geometry shader */
      if(!(sId = gl4duCreateShader(GL_GEOMETRY_SHADER, &filename[4]))) goto gl4duCreateProgram_ERROR;
      attachShader(*prg, *findidInShadersList(sId));
      fprintf(stderr, "\t\t%s : geometry shader\n", &filename[4]);
    } else { /* ??? shader */
      fprintf(stderr, "%s (%d): %s: erreur de syntaxe dans \"%s\"\n",
	      __FILE__, __LINE__, __func__, filename);
    }
  } while((filename = va_arg(pa, char *)) != NULL);
  va_end(pa);
  glLinkProgram(pId);
  gl4duPrintProgramInfoLog(pId, stderr);
  return pId;
 gl4duCreateProgram_ERROR:
  va_end(pa);
  deleteFromProgramsList(prg);
  return 0;
}

/*!\brief créé un program à partir d'une liste (variable) de nom de
 * fichiers shaders encapsulés dans un fichier crypté préalablement 
 * décrypté en ram et renvoie l'identifiant openGL du program créé.
 *
 * \ref encData fichier contenant un ensemble de shaders cryptés.
 * Les arguments sont les noms des fichiers shaders précédés d'un tag
 * indiquant le type de shader : <vs> pour un vertex shader et <fs>
 * pour un fragment shader et <gs> pour un geometry shader.
 *
 * \return l'identifiant openGL du program créé sinon 0. Dans ce
 * dernier cas les shaders créés (compilés) avant l'échec ne sont pas
 * supprimés ; un appel à \ref gl4duCleanUnattached peut s'en charger.
 */
GLuint gl4duCreateProgramFED(char * encData, char * firstone, ...) {
  static char * prevEncData = NULL, * decData = NULL;
  va_list  pa;
  char * filename;
  program_t ** prg;
  GLuint sId, pId = glCreateProgram();
  if(!pId) return pId;
  if(prevEncData != encData) {
	if(decData)
		free(decData);
	decData = aes_from_tar(prevEncData = encData);
  }
  prg = addInProgramsList(pId);

  filename = firstone;
  va_start(pa, firstone);
  fprintf(stderr, "%s (%d): Creation du programme %d a l'aide des Shaders :\n", __FILE__, __LINE__, pId);
  do {
    if(!strncmp("<vs>", filename, 4)) { /* vertex shader */
      if(!(sId = gl4duCreateShaderFED(decData, GL_VERTEX_SHADER, &filename[4]))) goto gl4duCreateProgram_ERROR;
      attachShader(*prg, *findidInShadersList(sId));
      fprintf(stderr, "\t\t%s : vertex shader\n", &filename[4]);
    } else if(!strncmp("<fs>", filename, 4)) { /* fragment shader */
      if(!(sId = gl4duCreateShaderFED(decData, GL_FRAGMENT_SHADER, &filename[4]))) goto gl4duCreateProgram_ERROR;
      attachShader(*prg, *findidInShadersList(sId));
      fprintf(stderr, "\t\t%s : fragment shader\n", &filename[4]);
    } else if(!strncmp("<gs>", filename, 4)) { /* geometry shader */
      if(!(sId = gl4duCreateShaderFED(decData, GL_GEOMETRY_SHADER, &filename[4]))) goto gl4duCreateProgram_ERROR;
      attachShader(*prg, *findidInShadersList(sId));
      fprintf(stderr, "\t\t%s : geometry shader\n", &filename[4]);
    } else { /* ??? shader */
      fprintf(stderr, "%s (%d): %s: erreur de syntaxe dans \"%s\"\n",
	      __FILE__, __LINE__, __func__, filename);
    }
  } while((filename = va_arg(pa, char *)) != NULL);
  va_end(pa);
  glLinkProgram(pId);
  gl4duPrintProgramInfoLog(pId, stderr);
  return pId;
 gl4duCreateProgram_ERROR:
  va_end(pa);
  deleteFromProgramsList(prg);
  return 0;
}

/*!\brief supprime le program dont l'identifiant openGL est \a id de la
 * liste de programs \ref programs_list.
 */
void gl4duDeleteProgram(GLuint id) {
  program_t ** prg = findInProgramsList(id);
  if(*prg) deleteFromProgramsList(prg);
}

/*!\brief supprime tous les programs et/ou tous les shaders.
 */
void gl4duClean(GL4DUenum what) {
  if(what & GL4DU_PROGRAM) {
    program_t ** ptr = &programs_list;
    while(*ptr)
      deleteFromProgramsList(ptr);
  }
  if(what & GL4DU_SHADER) {
    shader_t ** ptr = &shaders_list;
    while(*ptr)
      deleteFromShadersList(ptr);
  }
}

/*!\brief supprime programs et/ou shaders non liés. 
 */
void gl4duCleanUnattached(GL4DUenum what) {
  if(what & GL4DU_PROGRAM) {
    program_t ** ptr = &programs_list;
    while(*ptr) {
      if((*ptr)->nshaders <= 0) /* ne devrait pas être négatif */
	deleteFromProgramsList(ptr);
      else
	ptr = &((*ptr)->next);
    }
  }
  if(what & GL4DU_SHADER) {
    shader_t ** ptr = &shaders_list;
    while(*ptr) {
      if((*ptr)->nprograms <= 0) /* ne devrait pas être négatif */
	deleteFromShadersList(ptr);
      else
	ptr = &((*ptr)->next);
    }
  }
}

/*!\brief parcours la liste des shaders \ref shaders_list et vérifie
 * s'il y a besoin de mettre à jour le shader (recompiler et relinker).
 *
 * La vérification est effectuée sur la date de modification du
 * fichier representant le shader.
 *
 * \return 1 s'il y a eu une mise à jour (recompilation et relink)
 * sinon 0.
 *
 * \todo peut être ajouter un test sur le temps passé pour ne
 * parcourir la liste qu'un fois tous les 1/10 de secondes ????
*/
int gl4duUpdateShaders(void) {
  GLenum ot;
  char * fn;
  int maj = 0, i, n;
  struct stat buf;
  program_t ** p;
  shader_t ** ptr = &shaders_list;
#ifdef COMMERCIAL_V
  return 0;
#endif
  while(*ptr) {
    if(stat((*ptr)->filename, &buf) != 0) {
      fprintf(stderr, "%s:%d:In %s: erreur %d: %s\n",
	      __FILE__, __LINE__, __func__, errno, strerror(errno));
      return 0;
    }
    if((*ptr)->mod_time != buf.st_mtime) {
      if((n = (*ptr)->nprograms)) {
	p = malloc(n * sizeof * p);
	assert(p);
	memcpy(p, (*ptr)->programs, n * sizeof * p);
	for(i = 0; i < n; i++)
	  detachShader(p[i], *ptr);
	ot = (*ptr)->shadertype;
	fn = strdup((*ptr)->filename);
	deleteFromShadersList(ptr);
	ptr = addInShadersList(ot, fn);
	for(i = 0; i < n; i++) {
	  attachShader(p[i], *ptr);
	  glLinkProgram(p[i]->id);
	}
	free(p);
	free(fn);
      } else {
	ot = (*ptr)->shadertype;
	fn = strdup((*ptr)->filename);
	deleteFromShadersList(ptr);
	ptr = addInShadersList(ot, fn);
      }
      maj = 1;
    } else
      ptr = &((*ptr)->next);
  }
  return maj;
}

/*!\brief recherche un shader à partir du nom de fichier dans la liste
 * \ref shaders_list.
 *
 * Si le shader n'existe pas la fonction retourne NULL;
 *
 * \param filename le nom (le chemin entier (relatif)) du fichier
 * contenant le shader
 *
 * \return le pointeur de pointeur vers le shader.
 */
static shader_t ** findfnInShadersList(char * filename) {
  shader_t ** ptr = &shaders_list;
  while(*ptr) {
    if(!strcmp(filename, (*ptr)->filename))
      return ptr;
    ptr = &((*ptr)->next);
  }
  return ptr;
}

/*!\brief recherche un shader à partir de son identifiant openGL dans
 * la liste \ref shaders_list.
 *
 * Si le shader n'existe pas la fonction retourne NULL;
 *
 * \param id l'identifiant openGL du shader.
 *
 * \return le pointeur de pointeur vers le shader.
 */
static shader_t ** findidInShadersList(GLuint id) {
  shader_t ** ptr = &shaders_list;
  while(*ptr) {
    if(id == (*ptr)->id)
      return ptr;
    ptr = &((*ptr)->next);
  }
  return ptr;
}

/*!\brief ajoute un nouveau shader dans la liste de shaders \ref shaders_list.
 *
 * \return l'adresse du shader ajouté sinon NULL.
 */
static shader_t ** addInShadersList(GLenum shadertype, char * filename) {
  GLuint id;
  char * txt;
  struct stat buf;
  shader_t * ptr;
  if(!(id = glCreateShader(shadertype))) {
    fprintf(stderr, "%s (%d): %s: impossible de créer le shader\nglCreateShader a retourné 0\n", 
	    __FILE__, __LINE__, __func__);
    return NULL;
  }
  if(!(txt = gl4dReadTextFile(filename))) {
    glDeleteShader(id);
    return NULL;
  }
  if(stat(filename, &buf) != 0) {
    fprintf(stderr, "%s:%d:In %s: erreur %d: %s\n",
	    __FILE__, __LINE__, __func__, errno, strerror(errno));
    glDeleteShader(id);
    return NULL;
  }
  ptr = shaders_list;
  shaders_list = malloc(sizeof * shaders_list);
  assert(shaders_list);
  shaders_list->id         = id;
  shaders_list->shadertype = shadertype;
  shaders_list->filename   = strdup(filename);
  shaders_list->mod_time   = buf.st_mtime;
  shaders_list->todelete   = 0;
  shaders_list->nprograms  = 0;
  shaders_list->sprograms  = 2;
  shaders_list->next       = ptr;
  shaders_list->programs   = malloc(shaders_list->sprograms * sizeof shaders_list->programs);
  assert(shaders_list->programs);
  glShaderSource(id, 1, (const char **)&txt, NULL);
  glCompileShader(id);
  gl4duPrintShaderInfoLog(id, stderr);
  free(txt);
  return &shaders_list;
}

/*!\brief ajoute un nouveau shader dans la liste de shaders \ref shaders_list.
 * Version FED
 *
 * \return l'adresse du shader ajouté sinon NULL.
 */
static shader_t ** addInShadersListFED(char * decData, GLenum shadertype, char * filename) {
  GLuint id;
  char * txt;
  shader_t * ptr;
  if(!(id = glCreateShader(shadertype))) {
    fprintf(stderr, "%s (%d): %s: impossible de créer le shader\nglCreateShader a retourné 0\n", 
	    __FILE__, __LINE__, __func__);
    return NULL;
  }
  if(!(txt = gl4dExtractFromDecData(decData, filename))) {
    glDeleteShader(id);
    return NULL;
  }
  ptr = shaders_list;
  shaders_list = malloc(sizeof * shaders_list);
  assert(shaders_list);
  shaders_list->id         = id;
  shaders_list->shadertype = shadertype;
  shaders_list->filename   = strdup(filename);
  shaders_list->mod_time   = INT_MAX;
  shaders_list->todelete   = 0;
  shaders_list->nprograms  = 0;
  shaders_list->sprograms  = 2;
  shaders_list->next       = ptr;
  shaders_list->programs   = malloc(shaders_list->sprograms * sizeof shaders_list->programs);
  assert(shaders_list->programs);
  glShaderSource(id, 1, (const char **)&txt, NULL);
  glCompileShader(id);
  gl4duPrintShaderInfoLog(id, stderr);
  free(txt);
  return &shaders_list;
}

/*!\brief supprime le shader pointé par \a shp de la liste de shaders
 * \ref shaders_list.
 */
static void deleteFromShadersList(shader_t ** shp) {
  shader_t * ptr = *shp;
  *shp = (*shp)->next;
  free(ptr->filename);
  free(ptr->programs);
  glDeleteShader(ptr->id);
  free(ptr);
}

/*!\brief recherche un program à partir de son identifiant openGL dans
 * la liste \ref programs_list.
 *
 * Si le program n'existe pas la fonction retourne NULL;
 *
 * \return le pointeur de pointeur vers le program.
 */
static program_t ** findInProgramsList(GLuint id) {
  program_t ** ptr = &programs_list;
  while(*ptr) {
    if(id == (*ptr)->id)
      return ptr;
    ptr = &((*ptr)->next);
  }
  return ptr;
}

/*!\brief ajoute un nouveau program dans la liste de program \ref
 * programs_list.
 *
 * \return l'adresse du program ajouté sinon NULL.
 */
static program_t ** addInProgramsList(GLuint id) {
  program_t * ptr = programs_list;
  programs_list = malloc(sizeof * programs_list);
  assert(programs_list);
  programs_list->id       = id;
  programs_list->nshaders = 0;
  programs_list->sshaders = 4;
  programs_list->next     = ptr;
  programs_list->shaders  = malloc(programs_list->sshaders * sizeof * programs_list->shaders);
  assert(programs_list->shaders);
  return &programs_list;
}

/*!\brief supprime le program pointé par \a pp de la liste des programs
 * \ref programs_list.
 */
static void deleteFromProgramsList(program_t ** pp) {
  int i;
  program_t * ptr = *pp;
  *pp = (*pp)->next;
  for(i = 0; i < ptr->nshaders; i++)
    detachShader(ptr, ptr->shaders[i]);
  free(ptr->shaders);
  glDeleteProgram(ptr->id);
  free(ptr);
}

/*!\brief attache (lie) un program à un shader et vice versa.
 */
static void attachShader(program_t * prg, shader_t * sh) {
  int i;
  for(i = 0; i < prg->nshaders; i++)
    if(prg->shaders[i] == sh) return;
  for(i = 0; i < sh->nprograms; i++)
    if(sh->programs[i] == prg) return;
  if(prg->nshaders >= prg->sshaders) { /* ne devrait pas être supérieur */
    prg->shaders = realloc(prg->shaders, (prg->sshaders <<= 1) * sizeof * prg->shaders);
    assert(prg->shaders);
  }
  if(sh->nprograms >= sh->sprograms) { /* ne devrait pas être supérieur */
    sh->programs = realloc(sh->programs, (sh->sprograms <<= 1) * sizeof * sh->programs);
    assert(sh->programs);
  }
  prg->shaders[prg->nshaders++] =  sh;
  sh->programs[sh->nprograms++] = prg;
  glAttachShader(prg->id, sh->id);
}

/*!\brief détache (délie) un program à un shader et vice versa.
 */
static void detachShader(program_t * prg, shader_t * sh) {
  int i;
  glDetachShader(prg->id, sh->id);
  for(i = 0; i < prg->nshaders; i++)
    if(prg->shaders[i] == sh) break;
  for(prg->nshaders--; i < prg->nshaders; i++)
    prg->shaders[i] = prg->shaders[i + 1];
  for(i = 0; i < sh->nprograms; i++)
    if(sh->programs[i] == prg) break;
  for(sh->nprograms--; i < sh->nprograms; i++)
    sh->programs[i] = sh->programs[i + 1];
  if(sh->nprograms <= 0 && sh->todelete) /* ne devrait pas être négatif */
    deleteFromShadersList(findidInShadersList(sh->id));
}
