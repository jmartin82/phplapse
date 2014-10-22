/***********************************************************************************************************************
 *
 * Phplapse
 * ==========================================
 *
 * Copyright (C) 2014 (Jordi Martin)
 * http://jordimartin.es
 *
 ***********************************************************************************************************************
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 ***********************************************************************************************************************/

#ifndef PHP_PHPLAPSE_H
#define PHP_PHPLAPSE_H

#include <unistd.h>
#include <endian.h>
#include <byteswap.h>
#include <stdint.h>

#include "php.h"
#include "php_config.h"

#include "php_ini.h"
#include "php_globals.h"
#include "ext/standard/php_string.h"

#include "zend.h"
#include "zend_API.h"
#include "zend_execute.h"
#include "zend_compile.h"
#include "zend_extensions.h"

#define PHP_PHPLAPSE_VERSION "0.1"
#define PHP_PHPLAPSE_EXTNAME "phplapse"
 
extern zend_module_entry phplapse_module_entry;
#define phpext_phplapse_ptr &phplapse_module_entry

#define PHPLAPSE_CONTEXT_SIZE 5
#define PHPLAPSE_VAR_MAX_LEN 20
#define PHPLAPSE_MAX_FILENAME_LEN 70
#define PHPLAPSE_DATE_LEN 20
#define PHP_PHPLAPSE_FILE_VERSION 1

PHP_MINIT_FUNCTION(phplapse);
PHP_MSHUTDOWN_FUNCTION(phplapse);
PHP_RINIT_FUNCTION(phplapse);
PHP_RSHUTDOWN_FUNCTION(phplapse);
PHP_MINFO_FUNCTION(phplapse);

PHP_FUNCTION(phplapse_start);
PHP_FUNCTION(phplapse_stop);

struct _header {
  uint16_t version;
  unsigned char filename[PHPLAPSE_MAX_FILENAME_LEN];
  unsigned char datetime[PHPLAPSE_DATE_LEN];
  uint32_t steps;
}  __attribute__((__packed__));

typedef struct _header phplapse_header;

struct _step {
  uint32_t num; 
  uint32_t l_time;
  uint32_t a_time;
  uint32_t mem;
  uint32_t mem_peak;
  unsigned char f_name[PHPLAPSE_VAR_MAX_LEN];
  unsigned char c_name[PHPLAPSE_VAR_MAX_LEN];
  uint32_t context;
}  __attribute__((__packed__));

typedef struct _step phplapse_step;

static const phplapse_step empty_step = {
    0, //num
    0, //time
    0, //time
    0, //mem
    0, //peak
    "", //fname
    "", //cname
    0 //context
  };


ZEND_BEGIN_MODULE_GLOBALS(phplapse)
	zend_bool     do_lapse;
	unsigned char 	g_name[PHPLAPSE_VAR_MAX_LEN];
	php_stream *g_context_file;
	php_stream *g_idx_file;
	struct timeval g_start;
	struct timeval g_end;
	struct timeval g_total;
	uint32_t g_num;
  uint32_t g_context_position;
	phplapse_step g_step;
ZEND_END_MODULE_GLOBALS(phplapse)

#ifdef ZTS
#define PGV(v) TSRMG(phplapse_globals_id, zend_phplapse_globals *, v)
#else
#define PGV(v) (phplapse_globals.v)
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#        define TOLE16(x) (x)
#        define TOLE32(x) (x)
#        define TOLE64(x) (x)
#elif BYTE_ORDER == BIG_ENDIAN
#        define TOLE16(x) __bswap_16(x)
#        define TOLE32(x) __bswap_32(x)
#        define TOLE64(x) __bswap_64(x)
#else
#        error "Undefined host byte order (2)."
#endif


#endif /* PHP_PHPLAPSE_H */
