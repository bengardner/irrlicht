// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h


#include "CFileSystem.h"
#include "IReadFile.h"
#include "IWriteFile.h"
#include "CZipReader.h"
#include "CFileList.h"
#include "stdio.h"
#include "os.h"
#include "CReadFile.h"
#include "CMemoryFile.h"
#include "CLimitReadFile.h"
#include "CWriteFile.h"
#include <list>

#if defined (__STRICT_ANSI__)
    #error Compiling with __STRICT_ANSI__ not supported. g++ does set this when compiling with -std=c++11 or -std=c++0x. Use instead -std=gnu++11 or -std=gnu++0x. Or use -U__STRICT_ANSI__ to disable strict ansi.
#endif

#if defined (_IRR_WINDOWS_API_)
	#include <direct.h> // for _chdir
	#include <io.h> // for _access
	#include <tchar.h>
#elif (defined(_IRR_POSIX_API_) || defined(_IRR_OSX_PLATFORM_) || defined(_IRR_ANDROID_PLATFORM_))
		#include <stdio.h>
		#include <stdlib.h>
		#include <string.h>
		#include <limits.h>
		#include <sys/types.h>
		#include <dirent.h>
		#include <sys/stat.h>
		#include <unistd.h>
#elif defined(_IRR_EMSCRIPTEN_PLATFORM_)
    #include <unistd.h>
#endif

namespace irr
{
namespace io
{

//! constructor
CFileSystem::CFileSystem()
{
	#ifdef _DEBUG
	setDebugName("CFileSystem");
	#endif

	setFileListSystem(FILESYSTEM_NATIVE);
	//! reset current working directory
	getWorkingDirectory();

	ArchiveLoader.push_back(new CArchiveLoaderZIP(this));

}


//! destructor
CFileSystem::~CFileSystem()
{
	u32 i;

	for ( i=0; i < FileArchives.size(); ++i)
	{
		FileArchives[i]->drop();
	}

	for ( i=0; i < ArchiveLoader.size(); ++i)
	{
		ArchiveLoader[i]->drop();
	}
}


//! opens a file for read access
IReadFile* CFileSystem::createAndOpenFile(const io::path& filename)
{
	if ( filename.empty() )
		return 0;

	IReadFile* file = 0;
	u32 i;

	for (i=0; i< FileArchives.size(); ++i)
	{
		file = FileArchives[i]->createAndOpenFile(filename);
		if (file)
			return file;
	}

	// Create the file using an absolute path so that it matches
	// the scheme used by CNullDriver::getTexture().
	return CReadFile::createReadFile(getAbsolutePath(filename));
}


//! Creates an IReadFile interface for treating memory like a file.
IReadFile* CFileSystem::createMemoryReadFile(const void* memory, s32 len,
		const io::path& fileName, bool deleteMemoryWhenDropped)
{
	if (!memory)
		return 0;
	else
		return new CMemoryReadFile(memory, len, fileName, deleteMemoryWhenDropped);
}


//! Creates an IReadFile interface for reading files inside files
IReadFile* CFileSystem::createLimitReadFile(const io::path& fileName,
		IReadFile* alreadyOpenedFile, long pos, long areaSize)
{
	if (!alreadyOpenedFile)
		return 0;
	else
		return new CLimitReadFile(alreadyOpenedFile, pos, areaSize, fileName);
}


//! Creates an IReadFile interface for treating memory like a file.
IWriteFile* CFileSystem::createMemoryWriteFile(void* memory, s32 len,
		const io::path& fileName, bool deleteMemoryWhenDropped)
{
	if (!memory)
		return 0;
	else
		return new CMemoryWriteFile(memory, len, fileName, deleteMemoryWhenDropped);
}


//! Opens a file for write access.
IWriteFile* CFileSystem::createAndWriteFile(const io::path& filename, bool append)
{
	return CWriteFile::createWriteFile(filename, append);
}


//! Adds an external archive loader to the engine.
void CFileSystem::addArchiveLoader(IArchiveLoader* loader)
{
	if (!loader)
		return;

	loader->grab();
	ArchiveLoader.push_back(loader);
}

//! Returns the total number of archive loaders added.
u32 CFileSystem::getArchiveLoaderCount() const
{
	return ArchiveLoader.size();
}

//! Gets the archive loader by index.
IArchiveLoader* CFileSystem::getArchiveLoader(u32 index) const
{
	if (index < ArchiveLoader.size())
		return ArchiveLoader[index];
	else
		return 0;
}

//! move the hirarchy of the filesystem. moves sourceIndex relative up or down
bool CFileSystem::moveFileArchive(u32 sourceIndex, s32 relative)
{
	bool r = false;
	const s32 dest = (s32) sourceIndex + relative;
	const s32 dir = relative < 0 ? -1 : 1;
	const s32 sourceEnd = ((s32) FileArchives.size() ) - 1;
	IFileArchive *t;

	for (s32 s = (s32) sourceIndex;s != dest; s += dir)
	{
		if (s < 0 || s > sourceEnd || s + dir < 0 || s + dir > sourceEnd)
			continue;

		t = FileArchives[s + dir];
		FileArchives[s + dir] = FileArchives[s];
		FileArchives[s] = t;
		r = true;
	}
	return r;
}


//! Adds an archive to the file system.
bool CFileSystem::addFileArchive(const io::path& filename, bool ignoreCase,
			  bool ignorePaths, E_FILE_ARCHIVE_TYPE archiveType,
			  const core::stringc& password,
			  IFileArchive** retArchive)
{
	IFileArchive* archive = 0;
	bool ret = false;

	// see if archive is already added

	s32 i;

	// do we know what type it should be?
	if (archiveType == EFAT_UNKNOWN || archiveType == EFAT_FOLDER)
	{
		// try to load archive based on file name
		for (i = ArchiveLoader.size()-1; i >=0 ; --i)
		{
			if (ArchiveLoader[i]->isALoadableFileFormat(filename))
			{
				archive = ArchiveLoader[i]->createArchive(filename, ignoreCase, ignorePaths);
				if (archive)
					break;
			}
		}

		// try to load archive based on content
		if (!archive)
		{
			io::IReadFile* file = createAndOpenFile(filename);
			if (file)
			{
				for (i = ArchiveLoader.size()-1; i >= 0; --i)
				{
					file->seek(0);
					if (ArchiveLoader[i]->isALoadableFileFormat(file))
					{
						file->seek(0);
						archive = ArchiveLoader[i]->createArchive(file, ignoreCase, ignorePaths);
						if (archive)
							break;
					}
				}
				file->drop();
			}
		}
	}
	else
	{
		// try to open archive based on archive loader type

		io::IReadFile* file = 0;

		for (i = ArchiveLoader.size()-1; i >= 0; --i)
		{
			if (ArchiveLoader[i]->isALoadableFileFormat(archiveType))
			{
				// attempt to open file
				if (!file)
					file = createAndOpenFile(filename);

				// is the file open?
				if (file)
				{
					// attempt to open archive
					file->seek(0);
					if (ArchiveLoader[i]->isALoadableFileFormat(file))
					{
						file->seek(0);
						archive = ArchiveLoader[i]->createArchive(file, ignoreCase, ignorePaths);
						if (archive)
							break;
					}
				}
				else
				{
					// couldn't open file
					break;
				}
			}
		}

		// if open, close the file
		if (file)
			file->drop();
	}

	if (archive)
	{
		FileArchives.push_back(archive);
		if (password.size())
			archive->Password=password;
		if (retArchive)
			*retArchive = archive;
		ret = true;
	}
	else
	{
		os::Printer::log("Could not create archive for", filename, ELL_ERROR);
	}

	return ret;
}


bool CFileSystem::addFileArchive(IReadFile* file, bool ignoreCase,
		bool ignorePaths, E_FILE_ARCHIVE_TYPE archiveType,
		const core::stringc& password, IFileArchive** retArchive)
{
	if (!file || archiveType == EFAT_FOLDER)
		return false;

	if (file)
	{
		IFileArchive* archive = 0;
		s32 i;

		if (archiveType == EFAT_UNKNOWN)
		{
			// try to load archive based on file name
			for (i = ArchiveLoader.size()-1; i >=0 ; --i)
			{
				if (ArchiveLoader[i]->isALoadableFileFormat(file->getFileName()))
				{
					archive = ArchiveLoader[i]->createArchive(file, ignoreCase, ignorePaths);
					if (archive)
						break;
				}
			}

			// try to load archive based on content
			if (!archive)
			{
				for (i = ArchiveLoader.size()-1; i >= 0; --i)
				{
					file->seek(0);
					if (ArchiveLoader[i]->isALoadableFileFormat(file))
					{
						file->seek(0);
						archive = ArchiveLoader[i]->createArchive(file, ignoreCase, ignorePaths);
						if (archive)
							break;
					}
				}
			}
		}
		else
		{
			// try to open archive based on archive loader type
			for (i = ArchiveLoader.size()-1; i >= 0; --i)
			{
				if (ArchiveLoader[i]->isALoadableFileFormat(archiveType))
				{
					// attempt to open archive
					file->seek(0);
					if (ArchiveLoader[i]->isALoadableFileFormat(file))
					{
						file->seek(0);
						archive = ArchiveLoader[i]->createArchive(file, ignoreCase, ignorePaths);
						if (archive)
							break;
					}
				}
			}
		}

		if (archive)
		{
			FileArchives.push_back(archive);
			if (password.size())
				archive->Password=password;
			if (retArchive)
				*retArchive = archive;
			return true;
		}
		else
		{
			os::Printer::log("Could not create archive for", file->getFileName(), ELL_ERROR);
		}
	}

	return false;
}


//! Adds an archive to the file system.
bool CFileSystem::addFileArchive(IFileArchive* archive)
{
	if ( archive )
	{
		for (u32 i=0; i < FileArchives.size(); ++i)
		{
			if (archive == FileArchives[i])
			{
				return false;
			}
		}
		FileArchives.push_back(archive);
		archive->grab();

		return true;
	}

	return false;
}


//! removes an archive from the file system.
bool CFileSystem::removeFileArchive(u32 index)
{
	bool ret = false;
	if (index < FileArchives.size())
	{
		FileArchives[index]->drop();
		FileArchives.erase(index);
		ret = true;
	}
	return ret;
}


//! removes an archive from the file system.
bool CFileSystem::removeFileArchive(const io::path& filename)
{
	const path absPath = getAbsolutePath(filename);
	for (u32 i=0; i < FileArchives.size(); ++i)
	{
		if (absPath == FileArchives[i]->getFileList()->getPath())
			return removeFileArchive(i);
	}
	return false;
}


//! Removes an archive from the file system.
bool CFileSystem::removeFileArchive(const IFileArchive* archive)
{
	for (u32 i=0; i < FileArchives.size(); ++i)
	{
		if (archive == FileArchives[i])
		{
			return removeFileArchive(i);
		}
	}
	return false;
}


//! gets an archive
u32 CFileSystem::getFileArchiveCount() const
{
	return FileArchives.size();
}


IFileArchive* CFileSystem::getFileArchive(u32 index)
{
	return index < getFileArchiveCount() ? FileArchives[index] : 0;
}


//! Returns the string of the current working directory
const io::path& CFileSystem::getWorkingDirectory()
{
	EFileSystemType type = FileSystemType;

	if (type != FILESYSTEM_NATIVE)
	{
		type = FILESYSTEM_VIRTUAL;
	}
	else
	{
		#if defined(_IRR_WINDOWS_API_)
			fschar_t tmp[_MAX_PATH];
				_getcwd(tmp, _MAX_PATH);
				WorkingDirectory[FILESYSTEM_NATIVE] = tmp;
				WorkingDirectory[FILESYSTEM_NATIVE].replace('\\', '/');
		#endif

		#if (defined(_IRR_POSIX_API_) || defined(_IRR_OSX_PLATFORM_))

			// getting the CWD is rather complex as we do not know the size
			// so try it until the call was successful
			// Note that neither the first nor the second parameter may be 0 according to POSIX

				u32 pathSize=256;
				char *tmpPath = new char[pathSize];
				while ((pathSize < (1<<16)) && !(getcwd(tmpPath,pathSize)))
				{
					delete [] tmpPath;
					pathSize *= 2;
					tmpPath = new char[pathSize];
				}
				if (tmpPath)
				{
					WorkingDirectory[FILESYSTEM_NATIVE] = tmpPath;
					delete [] tmpPath;
				}
		#endif

		WorkingDirectory[type].validate();
	}

	return WorkingDirectory[type];
}


//! Changes the current Working Directory to the given string.
bool CFileSystem::changeWorkingDirectoryTo(const io::path& newDirectory)
{
	bool success=false;

	if (FileSystemType != FILESYSTEM_NATIVE)
	{
		WorkingDirectory[FILESYSTEM_VIRTUAL] = newDirectory;
		// is this empty string constant really intended?
		flattenFilename(WorkingDirectory[FILESYSTEM_VIRTUAL], _IRR_TEXT(""));
		success = true;
	}
	else
	{
		WorkingDirectory[FILESYSTEM_NATIVE] = newDirectory;

#if defined(_MSC_VER)
		success = (_chdir(newDirectory.c_str()) == 0);
#else
		success = (chdir(newDirectory.c_str()) == 0);
#endif
	}

	return success;
}


io::path CFileSystem::getAbsolutePath(const io::path& filename) const
{
	if ( filename.empty() )
		return filename;
#if defined(_IRR_WINDOWS_API_)
	fschar_t *p=0;
	fschar_t fpath[_MAX_PATH];
		p = _fullpath(fpath, filename.c_str(), _MAX_PATH);
		core::stringc tmp(p);
		tmp.replace('\\', '/');
	return tmp;
#elif (defined(_IRR_POSIX_API_) || defined(_IRR_OSX_PLATFORM_))
	c8* p=0;
	c8 fpath[4096];
	fpath[0]=0;
	p = realpath(filename.c_str(), fpath);
	if (!p)
	{
		// content in fpath is unclear at this point
		if (!fpath[0]) // seems like fpath wasn't altered, use our best guess
		{
			io::path tmp(filename);
			return flattenFilename(tmp);
		}
		else
			return io::path(fpath);
	}
	if (filename[filename.size()-1]=='/')
		return io::path(p)+_IRR_TEXT("/");
	else
		return io::path(p);
#else
	return io::path(filename);
#endif
}


//! returns the directory part of a filename, i.e. all until the first
//! slash or backslash, excluding it. If no directory path is prefixed, a '.'
//! is returned.
io::path CFileSystem::getFileDir(const io::path& filename) const
{
	// find last forward or backslash
	s32 lastSlash = filename.findLast('/');
	const s32 lastBackSlash = filename.findLast('\\');
	lastSlash = lastSlash > lastBackSlash ? lastSlash : lastBackSlash;

	if ((u32)lastSlash < filename.size())
		return filename.subString(0, lastSlash);
	else
		return _IRR_TEXT(".");
}


//! returns the base part of a filename, i.e. all except for the directory
//! part. If no directory path is prefixed, the full name is returned.
io::path CFileSystem::getFileBasename(const io::path& filename, bool keepExtension) const
{
	// find last forward or backslash
	s32 lastSlash = filename.findLast('/');
	const s32 lastBackSlash = filename.findLast('\\');
	lastSlash = core::max_(lastSlash, lastBackSlash);

	// get number of chars after last dot
	s32 end = 0;
	if (!keepExtension)
	{
		// take care to search only after last slash to check only for
		// dots in the filename
		end = filename.findLast('.');
		if (end == -1 || end < lastSlash)
			end=0;
		else
			end = filename.size()-end;
	}

	if ((u32)lastSlash < filename.size())
		return filename.subString(lastSlash+1, filename.size()-lastSlash-1-end);
	else if (end != 0)
		return filename.subString(0, filename.size()-end);
	else
		return filename;
}


//! flatten a path and file name for example: "/you/me/../." becomes "/you"
io::path& CFileSystem::flattenFilename(io::path& directory, const io::path& root) const
{
	directory.replace('\\', '/');
	if (directory.lastChar() != '/')
		directory.append('/');

	io::path dir;
	io::path subdir;

	s32 lastpos = 0;
	s32 pos = 0;
	bool lastWasRealDir=false;

	while ((pos = directory.findNext('/', lastpos)) >= 0)
	{
		subdir = directory.subString(lastpos, pos - lastpos + 1);

		if (subdir == _IRR_TEXT("../"))
		{
			if (lastWasRealDir)
			{
				deletePathFromPath(dir, 2);
				lastWasRealDir=(dir.size()!=0);
			}
			else
			{
				dir.append(subdir);
				lastWasRealDir=false;
			}
		}
		else if (subdir == _IRR_TEXT("/"))
		{
			dir = root;
		}
		else if (subdir != _IRR_TEXT("./"))
		{
			dir.append(subdir);
			lastWasRealDir=true;
		}

		lastpos = pos + 1;
	}
	directory = dir;
	return directory;
}


//! Get the relative filename, relative to the given directory
path CFileSystem::getRelativeFilename(const path& filename, const path& directory) const
{
	if ( filename.empty() || directory.empty() )
		return filename;

	io::path path1, file, ext;
	core::splitFilename(getAbsolutePath(filename), &path1, &file, &ext);
	io::path path2(getAbsolutePath(directory));
	std::list<io::path> list1, list2;
	path1.split(list1, _IRR_TEXT("/\\"), 2);
	path2.split(list2, _IRR_TEXT("/\\"), 2);
	std::list<io::path>::const_iterator it1,it2;
	it1=list1.begin();
	it2=list2.begin();

	#if defined (_IRR_WINDOWS_API_)
	fschar_t partition1 = 0, partition2 = 0;
	io::path prefix1, prefix2;
	if ( it1 != list1.end() )
		prefix1 = *it1;
	if ( it2 != list2.end() )
		prefix2 = *it2;
	if ( prefix1.size() > 1 && prefix1[1] == _IRR_TEXT(':') )
		partition1 = core::locale_lower(prefix1[0]);
	if ( prefix2.size() > 1 && prefix2[1] == _IRR_TEXT(':') )
		partition2 = core::locale_lower(prefix2[0]);

	// must have the same prefix or we can't resolve it to a relative filename
	if ( partition1 != partition2 )
	{
		return filename;
	}
	#endif


	for (; it1 != list1.end() && it2 != list2.end()
#if defined (_IRR_WINDOWS_API_)
		&& (io::path(*it1).make_lower()==io::path(*it2).make_lower())
#else
		&& (*it1==*it2)
#endif
		;)
	{
		++it1;
		++it2;
	}
	path1=_IRR_TEXT("");
	for (; it2 != list2.end(); ++it2)
		path1 += _IRR_TEXT("../");
	while (it1 != list1.end())
	{
		path1 += *it1++;
		path1 += _IRR_TEXT('/');
	}
	path1 += file;
	if (ext.size())
	{
		path1 += _IRR_TEXT('.');
		path1 += ext;
	}
	return path1;
}


//! Sets the current file systen type
EFileSystemType CFileSystem::setFileListSystem(EFileSystemType listType)
{
	EFileSystemType current = FileSystemType;
	FileSystemType = listType;
	return current;
}


//! Creates a list of files and directories in the current working directory
IFileList* CFileSystem::createFileList()
{
	CFileList* r = 0;
	io::path Path = getWorkingDirectory();
	Path.replace('\\', '/');
	if (!Path.empty() && Path.lastChar() != '/')
		Path.append('/');

	//! Construct from native filesystem
	if (FileSystemType == FILESYSTEM_NATIVE)
	{
		// --------------------------------------------
		//! Windows version
		#ifdef _IRR_WINDOWS_API_

		r = new CFileList(Path, true, false);

		// TODO: Should be unified once mingw adapts the proper types
#if defined(__GNUC__)
		long hFile; //mingw return type declaration
#else
		intptr_t hFile;
#endif

		struct _tfinddata_t c_file;
		if( (hFile = _tfindfirst( _T("*"), &c_file )) != -1L )
		{
			do
			{
				r->addItem(Path + c_file.name, 0, c_file.size, (_A_SUBDIR & c_file.attrib) != 0, 0);
			}
			while( _tfindnext( hFile, &c_file ) == 0 );

			_findclose( hFile );
		}

		#endif

		// --------------------------------------------
		//! Linux version
		#if (defined(_IRR_POSIX_API_) || defined(_IRR_OSX_PLATFORM_))


		r = new CFileList(Path, false, false);

		r->addItem(Path + _IRR_TEXT(".."), 0, 0, true, 0);

		//! We use the POSIX compliant methods instead of scandir
		DIR* dirHandle=opendir(Path.c_str());
		if (dirHandle)
		{
			struct dirent *dirEntry;
			while ((dirEntry=readdir(dirHandle)))
			{
				u32 size = 0;
				bool isDirectory = false;

				if((strcmp(dirEntry->d_name, ".")==0) ||
				   (strcmp(dirEntry->d_name, "..")==0))
				{
					continue;
				}
				struct stat buf;
				if (stat(dirEntry->d_name, &buf)==0)
				{
					size = buf.st_size;
					isDirectory = S_ISDIR(buf.st_mode);
				}
				#if !defined(_IRR_SOLARIS_PLATFORM_) && !defined(__CYGWIN__)
				// only available on some systems
				else
				{
					isDirectory = dirEntry->d_type == DT_DIR;
				}
				#endif

				r->addItem(Path + dirEntry->d_name, 0, size, isDirectory, 0);
			}
			closedir(dirHandle);
		}
		#endif
	}
	else
	{
		//! create file list for the virtual filesystem
		r = new CFileList(Path, false, false);

		//! add relative navigation
		SFileListEntry e2;
		SFileListEntry e3;

		//! PWD
		r->addItem(Path + _IRR_TEXT("."), 0, 0, true, 0);

		//! parent
		r->addItem(Path + _IRR_TEXT(".."), 0, 0, true, 0);

		//! merge archives
		for (u32 i=0; i < FileArchives.size(); ++i)
		{
			const IFileList *merge = FileArchives[i]->getFileList();

			for (u32 j=0; j < merge->getFileCount(); ++j)
			{
				if (core::isInSameDirectory(Path, merge->getFullFileName(j)) == 0)
				{
					r->addItem(merge->getFullFileName(j), merge->getFileOffset(j), merge->getFileSize(j), merge->isDirectory(j), 0);
				}
			}
		}
	}

	if (r)
		r->sort();
	return r;
}

//! Creates an empty filelist
IFileList* CFileSystem::createEmptyFileList(const io::path& path, bool ignoreCase, bool ignorePaths)
{
	return new CFileList(path, ignoreCase, ignorePaths);
}


//! determines if a file exists and would be able to be opened.
bool CFileSystem::existFile(const io::path& filename) const
{
	for (u32 i=0; i < FileArchives.size(); ++i)
		if (FileArchives[i]->getFileList()->findFile(filename)!=-1)
			return true;

#if defined(_MSC_VER)
		return (_access(filename.c_str(), 0) != -1);
#elif defined(F_OK)
		return (access(filename.c_str(), F_OK) != -1);
#else
	return (access(filename.c_str(), 0) != -1);
#endif
}


//! creates a filesystem which is able to open files from the ordinary file system,
//! and out of zipfiles, which are able to be added to the filesystem.
IFileSystem* createFileSystem()
{
	return new CFileSystem();
}


} // end namespace irr
} // end namespace io
