//
// "$Id$"
//
// More FileChooser routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1997-2000 by Easy Software Products.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
//
// Contents:
//
//   FileChooser::directory()  - Set the directory in the file chooser.
//   FileChooser::count()      - Return the number of selected files.
//   FileChooser::value()      - Return a selected filename.
//   FileChooser::up()         - Go up one directory.
//   FileChooser::newdir()     - Make a new directory.
//   FileChooser::rescan()     - Rescan the current directory.
//   FileChooser::fileListCB() - Handle clicks (and double-clicks) in the
//                                  FileBrowser.
//   FileChooser::fileNameCB() - Handle text entry in the FileBrowser.
//

//
// Include necessary headers.
//

#include <fltk/FileChooser.h>
#include <fltk/filename.h>
#include <fltk/ask.h>
#include <fltk/string.h>
#include <fltk/events.h>
#include <config.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fltk/utf.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
# include <windows.h>
# include <fltk/utf.h>
# undef _POSIX_
# include <direct.h>
# include <io.h>
# define access(a,b) _access(a,b)
# define R_OK 4
#else
# include <unistd.h>
# include <pwd.h>
#endif
#if (__MINGW32__)
#  include <ctype.h> // needed for isalpha()
#endif

using namespace fltk;

//
// 'FileChooser::directory()' - Set the directory in the file chooser.
//

void
FileChooser::directory(const char *d)	// I - Directory to change to
{
  char		pathname[1024],			// Full path of directory
		*pathptr,			// Pointer into full path
		*dirptr;			// Pointer into directory
  int		levels;				// Number of levels in directory


  // NULL == current directory
  if (d == NULL)
    d = ".";

  if (d[0] != '\0')
  {
    // Make the directory absolute...
#if defined(_WIN32) || defined(__EMX__)
    if (d[0] != '/' && d[0] != '\\' && d[1] != ':')
#else
    if (d[0] != '/' && d[0] != '\\')
#endif /* _WIN32 || __EMX__ */
      filename_normalize(directory_, sizeof(directory_), d, 0);
    else
    {
      strncpy(directory_, d, sizeof(directory_) - 1);
      directory_[sizeof(directory_) - 1] = '\0';
    }

    // Strip any trailing slash and/or period...
    dirptr = directory_ + strlen(directory_) - 1;
    if (*dirptr == '.')
      *dirptr-- = '\0';
    if ((*dirptr == '/' || *dirptr == '\\') && dirptr > directory_)
      *dirptr = '\0';
  }
  else
    directory_[0] = '\0';

  // Clear the directory menu and fill it as needed...
  dirMenu->clear();
#if defined(_WIN32) || defined(__EMX__)
  dirMenu->add("My Computer");
#else
  dirMenu->add("File Systems");
#endif /* _WIN32 || __EMX__ */

  levels = 0;
  for (dirptr = directory_, pathptr = pathname; *dirptr != '\0';)
  {
    if (*dirptr == '/' || *dirptr == '\\')
    {
      // Need to quote the slash first, and then add it to the menu...
      // Quoting is no longer needed with new menu->add() code, which
      // was changed to match fltk1.1
      // *pathptr++ = '\\';
      *pathptr++ = '/';
      *pathptr   = '\0';
      dirptr ++;

      dirMenu->add(pathname);
      levels ++;
    }
    else
      *pathptr++ = *dirptr++;
  }

  if (pathptr > pathname)
  {
    *pathptr = '\0';
    dirMenu->add(pathname);
    levels ++;
  }

  dirMenu->value(levels);
  dirMenu->redraw();

  // Rescan the directory...
  rescan();
}


//
// 'FileChooser::count()' - Return the number of selected files.
//

int				// O - Number of selected files
FileChooser::count()
{
  int		i;		// Looping var
  int		count;		// Number of selected files
  const char	*filename;	// Filename in input field or list
  char		pathname[1024];	// Full path to file


  if (type_ != MULTI)
  {
    // Check to see if the file name input field is blank...
    filename = fileName->value();
    if (filename == NULL || filename[0] == '\0')
      return (0);

    // Is the file name a directory?
    if (directory_[0] != '\0')
      snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
    else
    {
      strncpy(pathname, filename, sizeof(pathname) - 1);
      pathname[sizeof(pathname) - 1] = '\0';
    }

    if (filename_isdir(pathname))
      return (0);
    else
      return (1);
  }

  for (i = 0, count = 0; i < fileList->size(); i ++)
    if (fileList->selected(i))
    {
      // See if this file is a directory...
      filename = (char *)fileList->child(i)->label();
      if (directory_[0] != '\0')
	snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
      else
      {
        strncpy(pathname, filename, sizeof(pathname) - 1);
        pathname[sizeof(pathname) - 1] = '\0';
      }

      if (!filename_isdir(pathname))
	count ++;
    }

  return (count);
}


//
// 'FileChooser::value()' - Return a selected filename.
//

const char *			// O - Filename or NULL
FileChooser::value(int f)	// I - File number
{
  int		i;		// Looping var
  int		count;		// Number of selected files
  const char	*name;		// Current filename
  static char	pathname[1024];	// Filename + directory


  if (type_ != MULTI)
  {
    name = fileName->value();
    if (name[0] == '\0')
      return (NULL);

    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, name);
    return ((const char *)pathname);
  }

  for (i = 0, count = 0; i < fileList->size(); i ++)
    if (fileList->selected(i))
    {
      // See if this file is a directory...
      name = fileList->child(i)->label();
      snprintf(pathname, sizeof(pathname), "%s/%s", directory_, name);

      if (!filename_isdir(pathname))
      {
        // Nope, see if this this is "the one"...
	count ++;
	if (count == f)
          return ((const char *)pathname);
      }
    }

  return (NULL);
}


//
// 'FileChooser::value()' - Set the current filename.
//

void
FileChooser::value(const char *filename)	// I - Filename + directory
{
  int	i,					// Looping var
  	count;					// Number of items in list
  char	*slash;					// Directory separator
  char	pathname[1024];				// Local copy of filename


  // See if the filename is actually a directory...
  if (filename == NULL || filename_isdir(filename))
  {
    // Yes, just change the current directory...
    directory(filename);
    return;
  }

  if (!filename[0])
  {
    // Just show the current directory...
    directory(NULL);
    return;
  }

  // Switch to single-selection mode as needed
  if (type_ == MULTI)
    type(SINGLE);

  // See if there is a directory in there...
  strncpy(pathname, filename, sizeof(pathname) - 1);
  pathname[sizeof(pathname) - 1] = '\0';

  if ((slash = strrchr(pathname, '/')) == NULL)
    slash = strrchr(pathname, '\\');

  if (slash != NULL)
  {
    // Yes, change the display to the directory... 
    *slash++ = '\0';
    directory(pathname);
  }
  else
  {
    directory(NULL);
    slash = pathname;
  }

  // Set the input field to the remaining portion
  fileName->value(slash);
  fileName->position(0, strlen(slash));
  okButton->activate();

  // Then find the file in the file list and select it...
  count = fileList->size();

  for (i = 0; i < count; i ++)
    if (strcmp(fileList->child(i)->label(), slash) == 0)
    {
      fileList->select(i);
      break;
    }
}


//
// 'FileChooser::up()' - Go up one directory.
//

void
FileChooser::up()
{
  char *slash;		// Trailing slash


  if ((slash = strrchr(directory_, '/')) == NULL)
    slash = strrchr(directory_, '\\');

  if (directory_[0] != '\0')
    dirMenu->value(dirMenu->value() - 1);

  if (slash != NULL)
    *slash = '\0';
  else
  {
    upButton->deactivate();
    directory_[0] = '\0';
  }

  rescan();
}


//
// 'FileChooser::newdir()' - Make a new directory.
//

void
FileChooser::newdir()
{
  const char	*dir;		// New directory name
  char		pathname[1024];	// Full path of directory


  // Get a directory name from the user
  if ((dir = input("New Directory?")) == NULL)
    return;

  // Make it relative to the current directory as needed...
#if defined(_WIN32) || defined(__EMX__)
  if (dir[0] != '/' && dir[0] != '\\' && dir[1] != ':')
#else
  if (dir[0] != '/' && dir[0] != '\\')
#endif /* _WIN32 || __EMX__ */
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, dir);
  else
  {
    strncpy(pathname, dir, sizeof(pathname) - 1);
    pathname[sizeof(pathname) - 1] = '\0';
  }

  // Create the directory; ignore EEXIST errors...
#if defined(_WIN32) && !defined(__CYGWIN__)
  char namebuf[PATH_MAX];
  utf8tomb(pathname, strlen(pathname), namebuf, PATH_MAX);
  if (mkdir(namebuf))
#else
  if (mkdir(pathname, 0777))
#endif /* _WIN32 || __EMX__ */
    if (errno != EEXIST)
    {
      alert("Unable to create directory!");
      return;
    }

  // Show the new directory...
  directory(pathname);
}


//
// 'FileChooser::rescan()' - Rescan the current directory.
//

void
FileChooser::rescan()
{
  // Clear the current filename
  fileName->value("");
  okButton->deactivate();

  // Build the file list...
  fileList->load(directory_);
  fileList->redraw();
}


//
// 'FileChooser::fileListCB()' - Handle clicks (and double-clicks) in the
//                               FileBrowser.
//

void
FileChooser::fileListCB()
{
  char	filename[1024],		// New filename
	pathname[1024];		// Full pathname to file


  strncpy(filename, fileList->item()->label(), sizeof(filename) - 1);
  filename[sizeof(filename) - 1] = '\0';

#if defined(_WIN32) || defined(__EMX__)
  if (directory_[0] != '\0' && filename[0] != '/' && filename[0] != '\\' &&
      !(isalpha((uchar)filename[0]) && filename[1] == ':'))
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
  else
  {
    strncpy(pathname, filename, sizeof(pathname) - 1);
    pathname[sizeof(pathname) - 1] = '\0';
  }
#else
  if (directory_[0] != '\0' && filename[0] != '/')
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
  else
  {
    strncpy(pathname, filename, sizeof(pathname) - 1);
    pathname[sizeof(pathname) - 1] = '\0';
  }
#endif /* _WIN32 || __EMX__ */

  if (event_clicks() || event_key() == ReturnKey || event_key()==KeypadEnter)
  {
    if (filename_isdir(pathname))
    {
      fileList->deselect();
      directory(pathname);
      upButton->activate();
    } else {
      fileName->value(filename);
      window->hide();
    }
  }
  else
  {
    fileName->value(filename);
    okButton->activate();
  }
}


//
// 'FileChooser::fileNameCB()' - Handle text entry in the FileBrowser.
//

void
FileChooser::fileNameCB()
{
  char		*filename,	// New filename
		*slash,		// Pointer to trailing slash
		pathname[1024];	// Full pathname to file
  int		i,		// Looping var
		min_match,	// Minimum number of matching chars
		max_match,	// Maximum number of matching chars
		num_files,	// Number of files in directory
		first_line;	// First matching line
  const char	*file;		// File from directory


  // Get the filename from the text field...
  filename = (char *)fileName->value();

  if (filename == NULL || filename[0] == '\0')
  {
    okButton->deactivate();
    return;
  }

#if defined(_WIN32) || defined(__EMX__)
  if (directory_[0] != '\0' && filename[0] != '/' && filename[0] != '\\' &&
      !(isalpha((uchar)filename[0]) && filename[1] == ':'))
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
  else
  {
    strncpy(pathname, filename, sizeof(pathname) - 1);
    pathname[sizeof(pathname) - 1] = '\0';
  }
#else
  if (filename[0] == '~')
  {
    // Lookup user...
    struct passwd	*pwd;

    if (!filename[1] || filename[1] == '/')
      pwd = getpwuid(getuid());
    else
    {
      strncpy(pathname, filename + 1, sizeof(pathname) - 1);
      pathname[sizeof(pathname) - 1] = '\0';

      i = strlen(pathname) - 1;
      if (pathname[i] == '/')
        pathname[i] = '\0';

      pwd = getpwnam(pathname);
    }

    if (pwd)
    {
      strncpy(pathname, pwd->pw_dir, sizeof(pathname) - 1);
      pathname[sizeof(pathname) - 1] = '\0';

      if (filename[strlen(filename) - 1] == '/')
        strncat(pathname, "/", sizeof(pathname) - strlen(pathname) - 1);
    }
    else
      snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);

    endpwent();
  }
  else if (directory_[0] != '\0' && filename[0] != '/')
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
  else
  {
    strncpy(pathname, filename, sizeof(pathname) - 1);
    pathname[sizeof(pathname) - 1] = '\0';
  }
#endif /* _WIN32 || __EMX__ */

  if (event_key() == ReturnKey || event_key()==KeypadEnter)
  {
    // Enter pressed - select or change directory...
    if (filename_isdir(pathname))
      directory(pathname);
    else //if (type_ == CREATE || access(pathname, R_OK) == 0)
    {
      // New file or file exists...  If we are in multiple selection mode,
      // switch to single selection mode...
      if (type_ == MULTI)
        type(SINGLE);

      // Hide the window to signal things are done...
      window->hide();
    }
//  else
//  {
//    // File doesn't exist, so alert the user...
//    alert("Please choose an existing file!");
//   }
  }
  else if (event_key() != DeleteKey)
  {
    // Check to see if the user has entered a directory...
    if ((slash = strrchr(filename, '/')) == NULL)
      slash = strrchr(filename, '\\');

    if (slash != NULL)
    {
      // Yes, change directories and update the file name field...
      if ((slash = strrchr(pathname, '/')) == NULL)
	slash = strrchr(pathname, '\\');

      if (slash > pathname)		// Special case for "/"
        *slash++ = '\0';
      else
        slash++;

      if (strcmp(filename, "../") == 0)	// Special case for "../"
        up();
      else
        directory(pathname);

      // If the string ended after the slash, we're done for now...
      if (*slash == '\0')
        return;

      // Otherwise copy the remainder and proceed...
      fileName->value(slash);
      fileName->position(strlen(slash));
      filename = slash;
    }

    // Other key pressed - do filename completion as possible...
    num_files  = fileList->size();
    min_match  = strlen(filename);
    max_match  = 100000;
    first_line = 0;

    for (i = 0; i < num_files && max_match > min_match; i ++)
    {
      file = fileList->child(i)->label();

#if defined(_WIN32) || defined(__EMX__)
      if (strnicmp(filename, file, min_match) == 0)
#else
      if (strncmp(filename, file, min_match) == 0)
#endif // _WIN32 || __EMX__
      {
        // OK, this one matches; check against the previous match
	if (max_match == 100000)
	{
	  // First match; copy stuff over...
	  strncpy(pathname, file, sizeof(pathname) - 1);
	  pathname[sizeof(pathname) - 1] = '\0';
	  max_match = strlen(pathname);

	  // And then make sure that the item is visible
          fileList->topline(i);
	  first_line = i;
	}
	else
	{
	  // Succeeding match; compare to find maximum string match...
	  while (max_match > min_match)
#if defined(_WIN32) || defined(__EMX__)
	    if (strnicmp(file, pathname, max_match) == 0)
#else
	    if (strncmp(file, pathname, max_match) == 0)
#endif // _WIN32 || __EMX__
	      break;
	    else
	      max_match --;

          // Truncate the string as needed...
          pathname[max_match] = '\0';
	}
      }
    }

    fileList->deselect(0);
    fileList->redraw();

    // If we have any matches, add them to the input field...
    if (first_line > 0 && min_match == max_match &&
        max_match == (int)strlen(fileList->child(first_line)->label()))
      fileList->select(first_line);
    else if (max_match > min_match && max_match != 100000)
    {
      // Add the matching portion...
      fileName->replace(0, min_match, pathname, strlen(pathname));

      // Highlight it; if the user just pressed the backspace
      // key, position the cursor at the start of the selection.
      // Otherwise, put the cursor at the end of the selection so
      // s/he can press the right arrow to accept the selection
      // (Tab and End also do this for both cases.)
      const char *back, *end;
      end = pathname + strlen(pathname);
      back = utf8back(pathname + min_match - 1, pathname, end);
      //      if (back == pathname + min_match - 1) back--;
      if (event_key() == BackSpaceKey)
        fileName->position(back-pathname, max_match);
      else
        fileName->position(max_match, min_match);
    }

    // See if we need to enable the OK button...
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, fileName->value());

    if (type_ == CREATE || access(pathname, R_OK) == 0)
      okButton->activate();
    else
      okButton->deactivate();
  }
}


//
// End of "$Id$".
//