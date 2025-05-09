#!/bin/sh
skip=49

tab='	'
nl='
'
IFS=" $tab$nl"

umask=`umask`
umask 77

gztmpdir=
trap 'res=$?
  test -n "$gztmpdir" && rm -fr "$gztmpdir"
  (exit $res); exit $res
' 0 1 2 3 5 10 13 15

case $TMPDIR in
  / | /*/) ;;
  /*) TMPDIR=$TMPDIR/;;
  *) TMPDIR=/tmp/;;
esac
if type mktemp >/dev/null 2>&1; then
  gztmpdir=`mktemp -d "${TMPDIR}gztmpXXXXXXXXX"`
else
  gztmpdir=${TMPDIR}gztmp$$; mkdir $gztmpdir
fi || { (exit 127); exit 127; }

gztmp=$gztmpdir/$0
case $0 in
-* | */*'
') mkdir -p "$gztmp" && rm -r "$gztmp";;
*/*) gztmp=$gztmpdir/`basename "$0"`;;
esac || { (exit 127); exit 127; }

case `printf 'X\n' | tail -n +1 2>/dev/null` in
X) tail_n=-n;;
*) tail_n=;;
esac
if tail $tail_n +$skip <"$0" | gzip -cd > "$gztmp"; then
  umask $umask
  chmod 700 "$gztmp"
  (sleep 5; rm -fr "$gztmpdir") 2>/dev/null &
  "$gztmp" ${1+"$@"}; res=$?
else
  printf >&2 '%s\n' "Cannot decompress $0"
  (exit 127); res=127
fi; exit $res
�>9�gtest.sh �T]OA}�_q�nn$�����DHP�CH��N�5�]ܝ����1$"1j0*�J��t��'��3���q�q��sϜ9w"YEKd%��"0nJ��Pb�8��JM����@z!% D@T)���U�E�!`��:`�e�������/؏?T(�����P*)��F���I,�{+�%D�=Zi�/����5g{�q0�8��#5�����ÅV�]������͸�g���{g��2�|<g?���=n.}!�8o�ǚE��������V�Y����p�W�j�����̝�c�I���IU��)N'
1̈́Pf$*	�F���cabQ/�7��y8b���I3wg��y�����[�g���.��\�w��qh.�7~�7^؟��k�q�ܭ�{�n��ݣE��zkq���u6f�.�(y-�r�	+�t�1L�Y��YEN
]�M+���.��i�7��b�'���%�R"��i�a(�����R"�E�Wz�BD��L"'{��Qb<���e����J�%��w.^�<�
8�O�!�
�0UB�@���ͅ���XɺFN\�9L�\�X@� R�H�K���L;
�Cb���؀��O��')�OV�k�����+v���C�G>n�#}t����d�,����s=�"֞J��JX*QMr�x� ��SO��ni�ǭ��a�ƹ��o%���+�x�J�E�BV��N�t�����md�!��0�#�����AoO��eD��5�.�j��ѻb�$Fe1:(Fo��4��z!���%|{*�[��_�8����f~@��ld�;��4��M�?W{F�v���1��#�@  