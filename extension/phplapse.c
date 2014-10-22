#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h> 
#include <time.h>
#include <stdlib.h> 
#include "php.h"
#include "zend.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_extensions.h"
#include "phplapse.h"




//own functions
int unsigned phplapse_log(unsigned char *log);
ZEND_DLEXPORT void phplapse_statement_handler(zend_op_array *op_array);
int unsigned phplapse_extract_context(char *file_name, int unsigned line);
void phplapse_start_lapse_time();
void phplapse_stop_lapse_time();
static uint32_t phplapse_get_us_interval(struct timeval *start, struct timeval *end);
void phplapse_write_idx_header();


//---------------------------------------------------

// list of custom PHP functions provided by this extension
// set {NULL, NULL, NULL} as the last record to mark the end of list
static zend_function_entry phplapse_functions[] = {
  PHP_FE(phplapse_start, NULL)
  PHP_FE(phplapse_stop, NULL)
  {NULL, NULL, NULL}
};

// the following code creates an entry for the module and registers it with Zend.
zend_module_entry phplapse_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_PHPLAPSE_EXTNAME,
  phplapse_functions,
    PHP_MINIT(phplapse),               /* Module init callback */
    PHP_MSHUTDOWN(phplapse),           /* Module shutdown callback */
    PHP_RINIT(phplapse),               /* Request init callback */
    PHP_RSHUTDOWN(phplapse),           /* Request shutdown callback */
    PHP_MINFO(phplapse),               /* Module info callback */

#if ZEND_MODULE_API_NO >= 20010901
  PHP_PHPLAPSE_VERSION,
#endif
  STANDARD_MODULE_PROPERTIES
};


//INI CONFIG
PHP_INI_BEGIN()
PHP_INI_ENTRY("phplapse.output_dir", "/tmp", PHP_INI_ALL, NULL)
PHP_INI_END()

//GLOBALS
//http://php.net/manual/en/internals2.structure.globals.php
ZEND_DECLARE_MODULE_GLOBALS(phplapse)

/**
 * Module init callback.
 */
 PHP_MINIT_FUNCTION(phplapse) {
  REGISTER_INI_ENTRIES();
  return SUCCESS;
}

/**
 * Module shutdown callback.
 */
 PHP_MSHUTDOWN_FUNCTION(phplapse) {
  UNREGISTER_INI_ENTRIES();
  return SUCCESS;
}

/**
 * Request init callback.
 */
 PHP_RINIT_FUNCTION(phplapse) {
  PGV(do_lapse) = 0;
  return SUCCESS;
}

/**
 * Request shutdown callback.
 */
 PHP_RSHUTDOWN_FUNCTION(phplapse) {
  PGV(do_lapse) = 0;
  return SUCCESS;
}

/**
 * Module info callback. Returns the phplapse version.
 */
 PHP_MINFO_FUNCTION(phplapse)
 {
  php_info_print_table_start();
  php_info_print_table_header(2, "phplapse support", "enabled");
  php_info_print_table_row(2, "extension version", PHP_PHPLAPSE_VERSION);
  php_info_print_table_end();

  DISPLAY_INI_ENTRIES();
}


void gen_random(char *s, const int len) {
    
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    int i = 0;
    srand( (unsigned) time(NULL) * getpid());
    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}



// function implementation

// implementation of a custom phplapse_start()
PHP_FUNCTION(phplapse_start)
{
  if (PGV(do_lapse) == 0) {
    PGV(do_lapse) = 1;
    PGV(g_num) = 0;
    PGV(g_step) = empty_step;
    PGV(g_context_position) = 0;

    gen_random(PGV(g_name),PHPLAPSE_DATE_LEN);
    int file_name_size = strlen(PGV(g_name))+strlen(INI_STR("phplapse.output_dir"))+4;
    char idx_file_name[file_name_size];
    char context_file_name[file_name_size];
    php_sprintf(idx_file_name,"%s/%s.idx",INI_STR("phplapse.output_dir"),PGV(g_name));
    php_sprintf(context_file_name,"%s/%s.dat",INI_STR("phplapse.output_dir"),PGV(g_name));
    PGV(g_context_file) = php_stream_open_wrapper(context_file_name, "wb", REPORT_ERRORS, NULL);
    PGV(g_idx_file) = php_stream_open_wrapper(idx_file_name, "wb", REPORT_ERRORS, NULL);

    if (PGV(g_idx_file) == NULL)
    {
      zend_error(E_WARNING, "Error opening phplapse index file!\n");
      RETURN_FALSE;
    }
    if (PGV(g_context_file) == NULL)
    {
      zend_error(E_WARNING, "Error opening phplapse context file!\n");
      RETURN_FALSE;
    }

    //move the start of index file to leave enough space to put the header
    php_stream_seek(PGV(g_idx_file),sizeof(phplapse_header),SEEK_SET);

    phplapse_start_lapse_time();
    RETURN_TRUE;
  }
  RETURN_FALSE;
}

PHP_FUNCTION(phplapse_stop)
{
  if (PGV(do_lapse) == 1) {
    phplapse_write_idx_header();
    int file_name_size = strlen(PGV(g_name))+strlen(INI_STR("phplapse.output_dir"))+4;
    char idx_file_name[file_name_size];
    php_sprintf(idx_file_name,"%s/%s.idx",INI_STR("phplapse.output_dir"),PGV(g_name));

    PGV(do_lapse) = 0;
    if (PGV(g_idx_file) && PGV(g_context_file) )
    {
      php_stream_close(PGV(g_idx_file));
      php_stream_close(PGV(g_context_file));
      RETURN_STRING(idx_file_name,1);
    }
  }
  RETURN_FALSE;
}


int unsigned phplapse_extract_context(char *file_name, int unsigned line)
{


  int start_line = line-PHPLAPSE_CONTEXT_SIZE;
  int unsigned end_line = line+PHPLAPSE_CONTEXT_SIZE;
  
  FILE *fp;
  char buf[BUFSIZ];
  char context_output[1024];
  char filled_line[5] = "\n";
  int unsigned  i;
  int unsigned write = 0;
  int filled = 0;
  //write context data
  php_sprintf(context_output,"%s:%d\n", file_name,line);
  write +=phplapse_log(context_output);

  if ((fp = fopen(file_name, "r")) == NULL)
  {
    perror (file_name);
    return (EXIT_FAILURE);
  }
  
  //fill lines because can't get enough context before selected line
  while (start_line < 1) {
    write += phplapse_log(filled_line);
    filled++;
    start_line++;
  }

  //skip lines
  i = 1;
  while (fgets(buf, sizeof(buf), fp) != NULL)
  {
    if (i>=start_line && i<=end_line) {

      //fix last line 
      if (buf[(strlen(buf))-1] != '\n') {
        strcat(buf,"\n");
      }

      if (i == line){
        php_sprintf(context_output,"%4d##: %s", i, buf);
      }
      else {
        php_sprintf(context_output,"%4d  : %s", i, buf);
      }
      write += phplapse_log(context_output);
      filled++;
    }
    
    i++;
  }

  //fill lines because can't get enough context after selected line
  while (filled < (end_line - start_line)+1) {
    write += phplapse_log(filled_line);
    filled++;
  }

  fclose(fp);
  return write;    
}



int unsigned phplapse_log(unsigned char *log) {
  size_t len = strlen(log);
  if (PGV(g_context_file)) {
    return php_stream_write(PGV(g_context_file), log,len);
  }
  return -1;
}

void phplapse_write_idx_header(){
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  phplapse_header header = {0,"","",0};
  header.version = TOLE16(PHP_PHPLAPSE_FILE_VERSION);
  php_sprintf(header.filename,"%s.dat",PGV(g_name));  
  php_sprintf(header.datetime, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  header.steps = TOLE32(PGV(g_num-1));
  if (PGV(g_idx_file)) {
    php_stream_seek(PGV(g_idx_file),0,SEEK_SET);
    php_stream_write(PGV(g_idx_file), &header ,sizeof(phplapse_header));
  }
  
}




/**
 * Get time delta in microseconds.
 */
 static uint32_t phplapse_get_us_interval(struct timeval *start, struct timeval *end) {
  return (((end->tv_sec - start->tv_sec) * 1000000)
    + (end->tv_usec - start->tv_usec));
}

void phplapse_start_lapse_time() {
  struct timeval start;
  if (gettimeofday(&start, 0)) {
    zend_error(E_ERROR, "gettimeofday error!\n");
    return;
  }
  PGV(g_start) = start;
}

void phplapse_stop_lapse_time() {
  struct timeval end;
  if (gettimeofday(&end, 0)) {
    zend_error(E_ERROR, "gettimeofday error!\n");
    return;
  }
  PGV(g_end) = end;
}


static void phplapse_add_ns_interval(long incr) {
  PGV(g_total).tv_sec  += incr/1000000;
  PGV(g_total).tv_usec += incr%1000000;
  return;
}

ZEND_DLEXPORT void phplapse_statement_handler(zend_op_array *op_array)
{
  if (PGV(do_lapse)) {
    PGV(g_num)++;
    PGV(g_step) = empty_step;

    phplapse_stop_lapse_time();
    PGV(g_step).num = TOLE32(PGV(g_num));

    long acom = phplapse_get_us_interval(&PGV(g_start),&PGV(g_end));
    PGV(g_step).l_time = TOLE32(acom);
    phplapse_add_ns_interval(acom);

    PGV(g_step).a_time =  TOLE32(PGV(g_total).tv_sec * 1000000 + PGV(g_total).tv_usec);
    
    PGV(g_step).mem = TOLE32(zend_memory_usage(0 TSRMLS_CC)/1024);
    PGV(g_step).mem_peak = TOLE32(zend_memory_peak_usage(0 TSRMLS_CC)/1024);
    
    strncpy(PGV(g_step).f_name, get_active_function_name(TSRMLS_C), PHPLAPSE_VAR_MAX_LEN);
    strncpy(PGV(g_step).c_name, get_active_class_name(NULL TSRMLS_CC),PHPLAPSE_VAR_MAX_LEN);

    int write = phplapse_extract_context(zend_get_executed_filename(TSRMLS_C),
      zend_get_executed_lineno(TSRMLS_C)); 

    PGV(g_step).context = TOLE32(PGV(g_context_position));
    PGV(g_context_position) += write;
    

    if (PGV(g_idx_file)) {
      php_stream_write(PGV(g_idx_file), &PGV(g_step) ,sizeof(phplapse_step));

    }

    /*
    //debug only
    php_printf("S: %d\n",PGV(g_step).num);
    php_printf("ST: %d\n",PGV(g_step).l_time);
    php_printf("AT: %d\n",PGV(g_step).a_time);
    php_printf("M: %d\n",PGV(g_step).mem);
    php_printf("MP: %d\n",PGV(g_step).mem_peak);
    php_printf("F: %s\n",PGV(g_step).f_name);
    php_printf("C: %s\n",PGV(g_step).c_name);
    php_printf("CP: %d\n",PGV(g_step).context);
    php_printf("\n");
    */
    
    phplapse_start_lapse_time();
  }
}

ZEND_DLEXPORT void phplapse_call_coverage_zend_shutdown(zend_extension *extension)
{
}

int phplapse_call_coverage_zend_startup(zend_extension *extension)
{
  TSRMLS_FETCH();
  #if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || PHP_MAJOR_VERSION >= 6
  CG(compiler_options) = CG(compiler_options) | ZEND_COMPILE_EXTENDED_INFO;
  #else
    CG(extended_info) = 1;  /* XXX: this is ridiculous */
  #endif

  return zend_startup_module(&phplapse_module_entry);
}

#ifndef ZEND_EXT_API
#define ZEND_EXT_API  ZEND_DLEXPORT
#endif
ZEND_EXTENSION();

zend_extension zend_extension_entry = {
  PHP_PHPLAPSE_EXTNAME,
  PHP_PHPLAPSE_VERSION,
  "Jordi Martin",
  "@jordimartin", 
  "ASF2",
  phplapse_call_coverage_zend_startup,
  phplapse_call_coverage_zend_shutdown,
  NULL, /* activate_func_t */
  NULL, /* deactivate_func_t */
  NULL, /* message_handler_func_t */
  NULL, /* op_array_handler_func_t */
  phplapse_statement_handler, /* statement_handler_func_t */
  NULL, /* fcall_begin_handler_func_t */
  NULL, /* fcall_end_handler_func_t */
  NULL, /* op_array_ctor_func_t */
  NULL, /* op_array_dtor_func_t */
#ifdef COMPAT_ZEND_EXTENSION_PROPERTIES
  NULL, /* api_no_check */
  COMPAT_ZEND_EXTENSION_PROPERTIES
#else
  STANDARD_ZEND_EXTENSION_PROPERTIES
#endif
};


/* Init module */
ZEND_GET_MODULE(phplapse)
