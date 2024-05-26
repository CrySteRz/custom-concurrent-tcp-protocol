#pragma once
#include "protocol.hpp"
#include "packets.hpp"
#include <zstd.h>
#include <zlib.h>
#include <lzma.h>
#include <lz4.h>
#include <zip.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem> 

int get_compression_level(Level level, Format format) {
    switch (format) {
        case Format::ZSTD:
            switch (level) {
                case Level::FASTEST: return 1;
                case Level::FAST: return 3;
                case Level::NORMAL: return 6;
                case Level::GOOD: return 10;
                case Level::BEST: return 19;
                default: return 3;
            }
        case Format::GZIP:
            switch (level) {
                case Level::FASTEST: return Z_BEST_SPEED;
                case Level::FAST: return 3;
                case Level::NORMAL: return Z_DEFAULT_COMPRESSION;
                case Level::GOOD: return 7;
                case Level::BEST: return Z_BEST_COMPRESSION;
                default: return Z_DEFAULT_COMPRESSION;
            }
        case Format::XZ:
        case Format::LZMA:
            switch (level) {
                case Level::FASTEST: return 0;
                case Level::FAST: return 3;
                case Level::NORMAL: return 6;
                case Level::GOOD: return 7;
                case Level::BEST: return 9;
                default: return 6;
            }
        case Format::LZ4:
            switch (level) {
                case Level::FASTEST: return 0;
                case Level::FAST: return 1;
                case Level::NORMAL: return 1;
                case Level::GOOD: return 1;
                case Level::BEST: return 1;
                default: return 1;
            }
        case Format::ZIP:
            switch (level) {
                case Level::FASTEST: return 0;
                case Level::FAST: return 3;
                case Level::NORMAL: return 6;
                case Level::GOOD: return 7;
                case Level::BEST: return 9;
                default: return 6;
            }
        default:
            return 3;
    }
}

bool create_tar_archive(const std::vector<std::string> &files, const std::string &tar_path) {
    struct archive *a;
    struct archive_entry *entry;
    char buff[8192];
    int len;
    FILE *fp;

    a = archive_write_new();
    archive_write_add_filter_none(a);
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, tar_path.c_str());

    for (const auto &file_path : files) {
        entry = archive_entry_new();
        archive_entry_set_pathname(entry, file_path.c_str());
        archive_entry_set_size(entry, std::filesystem::file_size(file_path));
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(a, entry);
        fp = fopen(file_path.c_str(), "rb");
        while ((len = fread(buff, 1, sizeof(buff), fp)) > 0) {
            archive_write_data(a, buff, len);
        }
        fclose(fp);
        archive_entry_free(entry);
    }

    archive_write_close(a);
    archive_write_free(a);
    return true;
}

bool compress_with_zstd(const std::vector<std::string> &files, const std::string &archive_base_path, int compression_level) {
    std::string tar_path = archive_base_path + ".tar";
    if (!create_tar_archive(files, tar_path)) {
        return false;
    }

    std::string archive_path = archive_base_path + ".tar.zst";
    FILE *archive_file = fopen(archive_path.c_str(), "wb");
    if (!archive_file) return false;

    ZSTD_CCtx *cctx = ZSTD_createCCtx();
    if (!cctx) {
        fclose(archive_file);
        return false;
    }

    FILE *input_file = fopen(tar_path.c_str(), "rb");
    if (!input_file) {
        ZSTD_freeCCtx(cctx);
        fclose(archive_file);
        return false;
    }

    char in_buff[8192];
    char out_buff[ZSTD_compressBound(sizeof(in_buff))];
    size_t read_bytes;

    while ((read_bytes = fread(in_buff, 1, sizeof(in_buff), input_file)) > 0) {
        size_t compressed_size = ZSTD_compressCCtx(cctx, out_buff, sizeof(out_buff), in_buff, read_bytes, compression_level);
        if (ZSTD_isError(compressed_size)) {
            ZSTD_freeCCtx(cctx);
            fclose(input_file);
            fclose(archive_file);
            return false;
        }
        fwrite(out_buff, 1, compressed_size, archive_file);
    }

    fclose(input_file);
    ZSTD_freeCCtx(cctx);
    fclose(archive_file);
    remove(tar_path.c_str());

    return true;
}

bool compress_with_gzip(const std::vector<std::string> &files, const std::string &archive_base_path, int compression_level) {
    std::string tar_path = archive_base_path + ".tar";
    if (!create_tar_archive(files, tar_path)) {
        return false;
    }

    std::string archive_path = archive_base_path + ".tar.gz";
    FILE *archive_file = fopen(archive_path.c_str(), "wb");
    if (!archive_file) return false;

    gzFile gz = gzdopen(fileno(archive_file), "wb");
    if (!gz) {
        fclose(archive_file);
        return false;
    }

    gzsetparams(gz, compression_level, Z_DEFAULT_STRATEGY);

    char in_buff[8192];
    int read_bytes;

    FILE *input_file = fopen(tar_path.c_str(), "rb");
    if (!input_file) {
        gzclose(gz);
        return false;
    }

    while ((read_bytes = fread(in_buff, 1, sizeof(in_buff), input_file)) > 0) {
        if (gzwrite(gz, in_buff, read_bytes) != read_bytes) {
            fclose(input_file);
            gzclose(gz);
            return false;
        }
    }

    fclose(input_file);
    gzclose(gz);
    remove(tar_path.c_str());
    return true;
}

bool compress_with_xz(const std::vector<std::string> &files, const std::string &archive_base_path, int compression_level) {
    std::string tar_path = archive_base_path + ".tar";
    if (!create_tar_archive(files, tar_path)) {
        return false;
    }

    std::string archive_path = archive_base_path + ".tar.xz";
    FILE *archive_file = fopen(archive_path.c_str(), "wb");
    if (!archive_file) return false;

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_easy_encoder(&strm, compression_level, LZMA_CHECK_CRC64);
    if (ret != LZMA_OK) {
        fclose(archive_file);
        return false;
    }

    uint8_t in_buff[8192];
    uint8_t out_buff[8192];

    strm.next_out = out_buff;
    strm.avail_out = sizeof(out_buff);

    size_t read_bytes;
    FILE *input_file = fopen(tar_path.c_str(), "rb");
    if (!input_file) {
        lzma_end(&strm);
        fclose(archive_file);
        return false;
    }

    while ((read_bytes = fread(in_buff, 1, sizeof(in_buff), input_file)) > 0) {
        strm.next_in = in_buff;
        strm.avail_in = read_bytes;

        while (strm.avail_in != 0) {
            ret = lzma_code(&strm, LZMA_RUN);
            if (ret != LZMA_OK) {
                lzma_end(&strm);
                fclose(input_file);
                fclose(archive_file);
                return false;
            }

            fwrite(out_buff, 1, sizeof(out_buff) - strm.avail_out, archive_file);
            strm.next_out = out_buff;
            strm.avail_out = sizeof(out_buff);
        }
    }

    fclose(input_file);

    while (ret == LZMA_OK) {
        ret = lzma_code(&strm, LZMA_FINISH);
        fwrite(out_buff, 1, sizeof(out_buff) - strm.avail_out, archive_file);
        strm.next_out = out_buff;
        strm.avail_out = sizeof(out_buff);
    }

    lzma_end(&strm);
    fclose(archive_file);
    remove(tar_path.c_str());
    return ret == LZMA_STREAM_END;
}

bool compress_with_lz4(const std::vector<std::string> &files, const std::string &archive_base_path, int compression_level) {
    std::string tar_path = archive_base_path + ".tar";
    if (!create_tar_archive(files, tar_path)) {
        return false;
    }

    std::string archive_path = archive_base_path + ".tar.lz4";
    FILE *archive_file = fopen(archive_path.c_str(), "wb");
    if (!archive_file) return false;

    char in_buff[8192];
    char out_buff[LZ4_compressBound(sizeof(in_buff))];
    int read_bytes;

    FILE *input_file = fopen(tar_path.c_str(), "rb");
    if (!input_file) {
        fclose(archive_file);
        return false;
    }

    while ((read_bytes = fread(in_buff, 1, sizeof(in_buff), input_file)) > 0) {
        int compressed_size = LZ4_compress_default(in_buff, out_buff, read_bytes, sizeof(out_buff));
        if (compressed_size <= 0) {
            fclose(input_file);
            fclose(archive_file);
            return false;
        }
        fwrite(out_buff, 1, compressed_size, archive_file);
    }

    fclose(input_file);
    fclose(archive_file);
    remove(tar_path.c_str());
    return true;
}

bool compress_with_zip(const std::vector<std::string> &files, const std::string &archive_base_path, int compression_level) {
    std::string archive_path = archive_base_path + ".zip";
    printf("Creating zip archive at %s\n", archive_path.c_str());
    int error = 0;
    zip_t *zip = zip_open(archive_path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!zip) {
        return false;
    }

    for (const auto &file_path : files) {
        zip_source_t *source = zip_source_file(zip, file_path.c_str(), 0, 0);
        if (!source) {
            zip_close(zip);
            return false;
        }

        zip_int64_t idx = zip_file_add(zip, file_path.c_str(), source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_GUESS);
        if (idx < 0) {
            zip_source_free(source);
            zip_close(zip);
            return false;
        }

        zip_set_file_compression(zip, idx, compression_level, 0);
    }
    zip_close(zip);
    return true;
}