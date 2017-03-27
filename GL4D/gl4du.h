/*!\file gl4du.h
 *
 * \brief The GL4Dummies Utilities
 *
 * \author Farès BELHADJ amsi@grafeet.fr (ou amsi@ai.univ-paris8.fr)
 * \date March 28, 2008
 */

#ifndef _GL4DU_H
#define _GL4DU_H

#include "gl4dummies.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GL4DUenum GL4DUenum;
enum GL4DUenum {
  GL4DU_VERTEX_SHADER = 1,
  GL4DU_FRAGMENT_SHADER = 2,
  GL4DU_GEOMETRY_SHADER = 4,
  GL4DU_SHADER = 7,
  GL4DU_PROGRAM = 8,
  GL4DU_ALL = 0xffffffff
};

GL4DAPI void   GL4DAPIENTRY gl4duPrintShaderInfoLog(GLuint object, FILE * f);
GL4DAPI void   GL4DAPIENTRY gl4duPrintProgramInfoLog(GLuint object, FILE * f);
GL4DAPI void   GL4DAPIENTRY gl4duPrintFPS(FILE * fp);
GL4DAPI GLuint GL4DAPIENTRY gl4duCreateShader(GLenum shadertype, char * filename);
GL4DAPI GLuint GL4DAPIENTRY gl4duCreateShaderFED(char * decData, GLenum shadertype, char * filename);
GL4DAPI GLuint GL4DAPIENTRY gl4duFindShader(char * filename);
GL4DAPI void   GL4DAPIENTRY gl4duDeleteShader(GLuint id);
GL4DAPI GLuint GL4DAPIENTRY gl4duCreateProgram(char * firstone, ...);
GL4DAPI GLuint GL4DAPIENTRY gl4duCreateProgramFED(char * encData, char * firstone, ...);
GL4DAPI void   GL4DAPIENTRY gl4duDeleteProgram(GLuint id);
GL4DAPI void   GL4DAPIENTRY gl4duCleanUnattached(GL4DUenum what);
GL4DAPI void   GL4DAPIENTRY gl4duClean(GL4DUenum what);
GL4DAPI int    GL4DAPIENTRY gl4duUpdateShaders(void);

#ifdef __cplusplus
}
#endif

#endif
