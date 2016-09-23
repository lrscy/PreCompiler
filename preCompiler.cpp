#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <iostream>
#include <vector>
#include <stack>
#include <map>
using namespace std;
const int MAX_PATH_LEN = 512;
const int MAXN = 1 << 10;

char dirs_path[MAXN][MAX_PATH_LEN], filenames[MAXN][MAXN];
char cur_path[MAX_PATH_LEN];
int ndir;
bool noteflag, skip;
map<string, string> mp_define;
stack<string> st_define;

void get_dirs_path( int argc, char **argv ) {
    ndir = 0;
    for( int i = 1; i < argc; ++i ) {
        strcpy( dirs_path[ndir++], argv[i] );
    }
    if( ndir == 0 ) strcpy( dirs_path[ndir++], "." );
    return ;
}

bool isinclude( char *str, char *ret ) {
    char tc;
    bool flag = false;
    int pos, len, sp = 0;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    tc = str[sp + 7]; str[sp + 7] = 0;
    if( !strcmp( str + sp, "include" ) ) flag = true;
    str[sp + 7] = tc;
    if( !flag ) return false;
    pos = sp + 7;
    while( str[pos++] == ' ' ); --pos;
    len = strlen( str + pos );
    if( !( str[pos] == '"' && str[pos + len - 1] == '"' ) &&
        !( str[pos] == '<' && str[pos + len - 1] == '>' ) ) return false;
    ++pos; --len;
    tc = str[pos + len]; str[pos + len] = 0;
    strcpy( ret, str + pos );
    str[pos + len] = tc;
    return true;
}

int mfind( char *str ) {
    int pos = 0;
    stack<char> sc;
    while( str[pos] ) {
        if( sc.empty() && ( str[pos] == '\t' || str[pos] == ' ' ) ) break;
        else if( str[pos] == '(' ) sc.push( '(' );
        else if( !sc.empty() && str[pos] == ')' ) sc.pop();
        ++pos;
    }
    return pos;
}

bool isdefine( char *str, char *ret1, char *ret2 ) {
    char tc;
    bool flag = false;
    int pos = 0, len, sp;
    while( str[pos] == ' ' || str[pos] == '\t' ) ++pos;
    tc = str[pos + 6]; str[pos + 6] = 0;
    if( !strcmp( str + pos, "define" ) ) flag = true;
    str[pos + 6] = tc;
    if( !flag ) return false;
    sp = pos + 6;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    pos = mfind( str + sp ) + sp;
    tc = str[pos]; str[pos] = 0;
    strcpy( ret1, str + sp );
    str[pos] = tc;
    while( str[pos] == ' ' || str[pos] == '\t' ) ++pos;
    sp = pos;
    len = strlen( str );
    while( ( str[--len] == ' ' || str[len] == '\t' ) && len >= sp ); pos = len; str[pos + 1] = 0;
    strcpy( ret2, str + sp );
    if( !strcmp( ret2, "" ) ) strcpy( ret2, ret1 ); 
    return true;
}

bool isifdef( char *str, char *ret ) {
    char tc;
    bool flag = false;
    int pos = 0, sp;
    while( str[pos] == ' ' || str[pos] == '\t' ) ++pos;
    tc = str[pos + 5]; str[pos + 5] = 0;
    if( !strcmp( str + pos, "ifdef" ) ) flag = true;
    str[pos + 5] = tc;
    if( !flag ) return false;
    sp = pos + 5;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    pos = sp + mfind( str + sp );
    str[pos] = 0;
    strcpy( ret, str + sp );
    return true;
}

bool isundef( char *str, char *ret ) {
    char tc;
    bool flag = false;
    int pos = 0, sp;
    while( str[pos] == ' ' || str[pos] == '\t' ) ++pos;
    tc = str[pos + 5]; str[pos + 5] = 0;
    if( !strcmp( str + pos, "undef" ) ) flag = true;
    str[pos + 5] = tc;
    if( !flag ) return false;
    sp = pos + 5;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    pos = sp + mfind( str + sp );
    str[pos] = 0;
    strcpy( ret, str + sp );
    return true;
}

bool isifndef( char *str, char *ret ) {
    char tc;
    bool flag = false;
    int pos = 0, sp;
    while( str[pos] == ' ' || str[pos] == '\t' ) ++pos;
    tc = str[pos + 6]; str[pos + 6] = 0;
    if( !strcmp( str + pos, "ifndef" ) ) flag = true;
    str[pos + 6] = tc;
    if( !flag ) return false;
    sp = pos + 6;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    pos = sp + mfind( str + sp );
    str[pos] = 0;
    strcpy( ret, str + sp );
    return true;
}

bool isif( char *str, char *ret ) {
    char tc;
    bool flag = false;
    int sp = 0;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    tc = str[sp + 2]; str[sp + 2] = 0;
    if( !strcmp( str + sp, "if" ) ) flag = true;
    str[sp + 2] = tc;
    if( !flag ) return false;
    sp = pos + 2;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    pos = sp + mfind( str + sp );
    str[pos] = 0;
    strcpy( ret, str + sp );
    return flag;
}

bool iselif( char *str ) {
    char tc;
    bool flag = false;
    int sp = 0;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    tc = str[sp + 4]; str[sp + 4] = 0;
    if( !strcmp( str + sp, "elif" ) ) flag = true;
    str[sp + 4] = tc;
    return flag;
}

bool iselse( char *str ) {
    char tc;
    bool flag = false;
    int sp = 0;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    tc = str[sp + 4]; str[sp + 4] = 0;
    if( !strcmp( str + sp, "else" ) ) flag = true;
    str[sp + 4] = tc;
    return flag;
}

bool isendif( char *str ) {
    char tc;
    bool flag = false;
    int sp = 0;
    while( str[sp] == ' ' || str[sp] == '\t' ) ++sp;
    tc = str[sp + 5]; str[sp + 5] = 0;
    if( !strcmp( str + sp, "endif" ) ) flag = true;
    str[sp + 5] = tc;
    return flag;
}

void dodefine( char *str, char *str2 ) {
    if( !skip ) mp_define[str] = str2;
    return ;
}

void doundef( char *str ) {
    if( !skip && mp_define.find( str ) != mp_define.end() )
        mp_define.erase( str );
    return ;
}

bool doif( char *str ) {
    int tmp;
    st_define.push( str );
    if( skip ) return false;
    if( ~sscanf( str, "%d", &tmp ) ) return false;
    if( tmp == 0 ) return false;
    return true;
}

bool doelif() {
    return true;
}

bool doelse() {
    return true;
}

bool doifdef( char *str ) {
    bool flag = !( mp_define.find( str ) == mp_define.end() );
    st_define.push( str );
    if( skip ) return false;
    return flag;
}

bool doifndef( char *str ) {
    bool flag = ( mp_define.find( str ) == mp_define.end() );
    st_define.push( str );
    if( skip ) return false;
    return flag;
}

bool doendif() {
    if( st_define.empty() ) return false;
    st_define.pop();
    return true;
}

bool process( FILE *rfp, char *filename ) {
    FILE *fp;
    char line[MAXN], tline[MAXN], path[MAXN] = "/usr/include/";
    char str[MAXN], str2[MAXN];
    bool flag = false;
    int len = strlen( filename );
    if( filename[len - 1] == '"' ) {
        filename[len - 1] = 0;
        fp = fopen( filename, "r" );
        if( fp == NULL ) {
            strcat( path, filename );
            fp = fopen( path, "r" );
        }
    } else if( filename[len - 1] == '>' ) {
        filename[len - 1] = 0;
        strcat( path, filename );
        fp = fopen( path, "r" );
    } else {
        fp = fopen( filename, "r" );
    }
    if( fp == NULL ) return true;
    while( fgets( line, MAXN, fp ) != NULL) {
        len = strlen( line );
        if( len == 0 ) continue;
        line[len - 1] = 0;
        memset( str, 0, sizeof( str ) );
        --len;
        if( line[len - 1] == '\\' ) {
            line[len - 1] = 0;
            while( fgets( tline, MAXN, fp ) != NULL ) {
                len = strlen( tline ); --len;
                tline[len] = 0;
                if( tline[len - 1] != '\\' ) { strcat( line, tline ); break; }
                else { tline[--len] = 0; strcat( line, tline ); }
            }
        }
        if( line[0] == '#' ) {
            if( !skip && isinclude( line + 1, str ) ) {
                if( !process( rfp, str ) ) return false;
            } else if( isdefine( line + 1, str, str2 ) ) {
                dodefine( str, str2 );
            } else if( isifndef( line + 1, str ) ) {
                if( !doifndef( str ) ) skip = true;
            } else if( isifdef( line + 1, str ) ) {
                if( !doifdef( str ) ) skip = true;
            } else if( isendif( line + 1 ) ) {
                if( !doendif() ) return false;
                skip = false;
            } else if( isundef( line + 1, str ) ) {
                doundef( str );
            } else if( isif( line + 1, str ) ) {
                if( !doif( str ) ) skip = true;
            } else if( iselif( line + 1 ) ) {
                doelif();
            } else if( iselse( line + 1 ) ) {
                doelse();
            }
        } else {
            if( skip ) continue;
            fprintf( rfp, "%s\n", line );
        }
    }
    fclose( fp );
    return true;
}

bool combine( char *path ) {
    DIR *pdir;
    struct dirent *filenames_dirent;
    FILE *fp = fopen( "tmp.tmp", "w+" ), *tfp;
    char str[MAXN], *pstr;
    int nfile = 0;
    if( ( pdir = opendir( path ) ) == NULL ) {
        fprintf( stderr, "ls1: cannot open %s, not a directory.\n", path );
        fclose( fp );
        return false;
    }
    while( ( filenames_dirent = readdir( pdir ) ) != NULL ) {
        if( !strcmp( filenames_dirent->d_name, "." ) || !strcmp( filenames_dirent->d_name, ".." ) ) continue;
        int len = strlen( filenames_dirent->d_name );
        pstr = strrchr( filenames_dirent->d_name, '.' );
        if( pstr == NULL ) continue;
        strcpy( str, pstr + 1 );
        if( !strcmp( str, "cpp" ) || !strcmp( str, "c" ) || !strcmp( str, "h" ) ) {
            strcpy( filenames[nfile++], filenames_dirent->d_name );
        }
    }
    closedir( pdir );
    int cnt = 0, No;
    char cstr[MAXN];
    string tstr;
    for( int i = 0; i < nfile; ++i ) {
        tfp = fopen( filenames[i], "r" );
        while( fgets( cstr, MAXN, tfp ) != NULL ) {
            int pos;
            tstr = cstr;
            if( ( pos = tstr.find( "main" ) ) != string::npos && tstr[pos + 4] != '"' ) {
                No = i; ++cnt;
            }
        }
        fclose( tfp );
    }
    if( cnt != 1 ) { fclose( fp ); return false; }
    skip = false;
    if( !process( fp, filenames[No] ) ) { fclose( fp ); return false; }
    fclose( fp );
    return true;
}

bool noteProcess( string &str ) {
    while( true ) {
        int p1 = str.find( "//" );
        int p2 = str.find( "/*" );
        int p3 = str.find( "*/" );
        if( p1 == -1 && p2 == -1 && p3 == -1 ) break;
        int minp = str.length();
        if( ~p1 ) minp = min( minp, p1 );
        if( ~p2 ) minp = min( minp, p2 );
        if( ~p3 ) minp = min( minp, p3 );
        if( noteflag ) {
            if( p3 == -1 ) { str = ""; break; }
            else {
                noteflag = false;
                str = str.substr( p3 + 2, str.length() - p3 - 2 );
            }
        } else {
            if( minp == p3 ) {
                noteflag = false;
                return false;
            } else if( minp == p2 ) {
                if( p3 == -1 ) { str = str.substr( 0, p2 ); noteflag = true; }
                else str = str.substr( 0, p2 ) + str.substr( p3 + 2, str.length() - p3 - 2 );
            } else if( minp == p1 ) {
                str = str.substr( 0, p1 );
            }
        }
    }
    return true;
}

bool notesProcess() {
    FILE *fp1, *fp2;
    char line[MAXN];
    bool flag = true;
    fp1 = fopen( "tmp.tmp", "r" );
    fp2 = fopen( "pre_compiler.c", "w+" );
    while( fgets( line, MAXN, fp1 ) != NULL ) {
        int len = strlen( line ); line[--len] = 0;
        string tstr = line;
        if( !noteProcess( tstr ) ) {
            flag = false;
            break;
        }
        if( tstr != "" && !noteflag ) fprintf( fp2, "%s\n", tstr.c_str() );
    }
    fclose( fp1 );
    fclose( fp2 );
    if( !flag ) {
        fp2 = fopen( "pre_compiler.tmp", "w+" );
        fclose( fp2 );
    }
    return true;
}

bool pre_compile( char *path ) {
    noteflag = false;
    if( !combine( path ) ) return false;
    if( !notesProcess() ) return false;
    return true;
}

int main( int argc, char **argv ) {
    get_dirs_path( argc, argv );
    for( int i = 0; i < ndir; ++i ) {
        if( !pre_compile( dirs_path[i] ) ) puts( "Error!" );
        else puts( "OK!" );
        if( i != ndir - 1 ) puts( "" );
    }
    return 0;
}
