#include <chainparams.h>
#include <fs.h>
#include <logging.h>
#include <util/system.h>
#include <wallet/db.h>
#include <exception>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>
namespace wallet {
std::vector<fs::path> ListDatabases(const fs::path& wallet_dir)
{
    std::vector<fs::path> paths;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(wallet_dir, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            if (fs::is_directory(*it)) {
                it.disable_recursion_pending();
                LogPrintf("%s: %s %s -- skipping.\n", __func__, ec.message(), fs::PathToString(it->path()));
            } else {
                LogPrintf("%s: %s %s\n", __func__, ec.message(), fs::PathToString(it->path()));
            }
            continue;
        }
        try {
            const fs::path path{it->path().lexically_relative(wallet_dir)};
            if (it->status().type() == fs::file_type::directory &&
                (IsBDBFile(BDBDataFile(it->path())) || IsSQLiteFile(SQLiteDataFile(it->path())))) {
                paths.emplace_back(path);
            } else if (it.depth() == 0 && it->symlink_status().type() == fs::file_type::regular && IsBDBFile(it->path())) {
                if (it->path().filename() == "wallet.dat") {
                    paths.emplace_back();
                } else {
                    paths.emplace_back(path);
                }
            }
        } catch (const std::exception& e) {
            LogPrintf("%s: Error scanning %s: %s\n", __func__, fs::PathToString(it->path()), e.what());
            it.disable_recursion_pending();
        }
    }
    return paths;
}
fs::path BDBDataFile(const fs::path& wallet_path)
{
    if (fs::is_regular_file(wallet_path)) {
        return wallet_path;
    } else {
        return wallet_path / "wallet.dat";
    }
}
fs::path SQLiteDataFile(const fs::path& path)
{
    return path / "wallet.dat";
}
bool IsBDBFile(const fs::path& path)
{
    if (!fs::exists(path)) return false;
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) LogPrintf("%s: %s %s\n", __func__, ec.message(), fs::PathToString(path));
    if (size < 4096) return false;
    std::ifstream file{path, std::ios::binary};
    if (!file.is_open()) return false;
    file.seekg(12, std::ios::beg);
    uint32_t data = 0;
    file.read((char*) &data, sizeof(data));
    return data == 0x00053162 || data == 0x62310500;
}
bool IsSQLiteFile(const fs::path& path)
{
    if (!fs::exists(path)) return false;
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) LogPrintf("%s: %s %s\n", __func__, ec.message(), fs::PathToString(path));
    if (size < 512) return false;
    std::ifstream file{path, std::ios::binary};
    if (!file.is_open()) return false;
    char magic[16];
    file.read(magic, 16);
    file.seekg(68, std::ios::beg);
    char app_id[4];
    file.read(app_id, 4);
    file.close();
    std::string magic_str(magic, 16);
    if (magic_str != std::string("SQLite format 3", 16)) {
        return false;
    }
    return memcmp(Params().MessageStart(), app_id, 4) == 0;
}
void ReadDatabaseArgs(const ArgsManager& args, DatabaseOptions& options)
{
    options.use_unsafe_sync = args.GetBoolArg("-unsafesqlitesync", options.use_unsafe_sync);
    options.use_shared_memory = !args.GetBoolArg("-privdb", !options.use_shared_memory);
    options.max_log_mb = args.GetIntArg("-dblogsize", options.max_log_mb);
}
}
