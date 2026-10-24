/* CubalC hostops monolit dispatcher — peels only (B_hostops_margin).
 * Cube is SoT. Zero bloat. Bodies live in shards.
 *
 * exists·path_kind·read·write → cubalc_hostops_io.c
 * protect/http/spawn → cubalc_hostops_httpspawn.c
 * mtime·atime·ctime → cubalc_hostops_stat_time.c
 * rm·rmdir·rename·can_* → cubalc_hostops_fs_mut.c
 * umask·pushd·popd·dirstack → cubalc_hostops_dirstack.c
 * relpath·kindstr·abspath·join → cubalc_hostops_path.c
 * groups·ingroup·copy → cubalc_hostops_groupscopy.c
 * readlink·islink → cubalc_hostops_link.c
 * ownername·groupname·mode·chmod → cubalc_hostops_ownermode.c
 * touch·listdir·mkdir → cubalc_hostops_fs_basic.c
 * env·env_set·env_unset → cubalc_hostops_env.c
 * find_cubalc·which → cubalc_hostops_which.c
 * json_* · chat → json_*.c · hostops_chat.c
 */
typedef int cubalc_hostops_peel_anchor;
