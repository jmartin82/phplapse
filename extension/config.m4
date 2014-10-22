PHP_ARG_ENABLE(phplapse, whether to enable phplapse support,
[ --enable-phplapse      Enable phplapse support])

if test "$PHP_PHLAPSE" != "no"; then
  PHP_NEW_EXTENSION(phplapse, phplapse.c, $ext_shared)
fi
