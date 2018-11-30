#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "simdjson/jsonparser.h"

/**
 * Does the file filename ends with the given extension.
 */
static bool hasExtension(const char *filename, const char *extension) {
  const char *ext = strrchr(filename, '.');
  return (ext && !strcmp(ext, extension));
}

bool startsWith(const char *pre, const char *str) {
  size_t lenpre = strlen(pre), lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

bool contains(const char *pre, const char *str) {
  return (strstr(str, pre) != NULL);
}


bool validate(const char *dirname) {
  bool everythingfine = true;
  // init_state_machine(); // no longer necessary
  const char *extension = ".json";
  size_t dirlen = strlen(dirname);
  struct dirent **entry_list;
  int c = scandir(dirname, &entry_list, 0, alphasort);
  if (c < 0) {
    printf("error accessing %s \n", dirname);
    return false;
  }
  if (c == 0) {
    printf("nothing in dir %s \n", dirname);
    return false;
  }
  bool * isfileasexpected = new bool[c];
  for(int i = 0; i < c; i++) isfileasexpected[i] = true;
  size_t howmany = 0;
  bool needsep = (strlen(dirname) > 1) && (dirname[strlen(dirname) - 1] != '/');
  for (int i = 0; i < c; i++) {
    const char *name = entry_list[i]->d_name;
    if (hasExtension(name, extension)) {
      //printf("validating: file %s \n", name);
      size_t filelen = strlen(name);
      char *fullpath = (char *)malloc(dirlen + filelen + 1 + 1);
      strcpy(fullpath, dirname);
      if (needsep) {
        fullpath[dirlen] = '/';
        strcpy(fullpath + dirlen + 1, name);
      } else {
        strcpy(fullpath + dirlen, name);
      }
      std::pair<u8 *, size_t> p;
      try {
        p = get_corpus(fullpath);
      } catch (const std::exception& e) { 
        std::cout << "Could not load the file " << fullpath << std::endl;
        return EXIT_FAILURE;
      }
      ParsedJson *pj_ptr = allocate_ParsedJson(p.second, 1024);
      if(pj_ptr == NULL) {
        std::cerr<< "can't allocate memory"<<std::endl;
        return false;
      }
      ++howmany;
      ParsedJson &pj(*pj_ptr);
      bool isok = json_parse(p.first, p.second, pj);
      if(contains("EXCLUDE",name)) {
        // skipping
        howmany--;
      } else if (startsWith("pass", name)) {
        if (!isok) {
          isfileasexpected[i] = false;
          printf("warning: file %s should pass but it fails.\n", name);
          everythingfine = false;
        }
      } else if (startsWith("fail", name)) {
        if (isok) {
          isfileasexpected[i] = false;
          printf("warning: file %s should fail but it passes.\n", name);
          everythingfine = false;
        }
      } else {
        printf("File %s %s.\n", name,
               isok ? " is valid JSON " : " is not valid JSON");
      }
      free(p.first);
      free(fullpath);
      deallocate_ParsedJson(pj_ptr);
    }
  }
  printf("%zu files checked.\n", howmany);
  if(everythingfine) {
    printf("All ok!\n");
  } else {
    printf("There were problems! Consider reviewing the following files:\n");
    for(int i = 0; i < c; i++) {
      if(!isfileasexpected[i]) printf("%s \n", entry_list[i]->d_name);
    }
  }
  for (int i = 0; i < c; ++i)
    free(entry_list[i]);
  free(entry_list);
  delete[] isfileasexpected;

  return everythingfine;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <directorywithjsonfiles>"
              << std::endl;
    std::cout
        << "We are going to assume you mean to use the 'jsonchecker' directory."
        << std::endl;
    return validate("jsonchecker/") ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  return validate(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
