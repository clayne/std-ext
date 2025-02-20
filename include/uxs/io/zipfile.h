#pragma once

#include "iodevice.h"
#include "ziparch.h"

namespace uxs {

class UXS_EXPORT_ALL_STUFF_FOR_GNUC zipfile : public iodevice {
 public:
    zipfile() noexcept = default;
    zipfile(ziparch& arch, const char* fname) { open(arch, fname); }
    zipfile(ziparch& arch, const wchar_t* fname) { open(arch, fname); }
    ~zipfile() override { close(); }
    zipfile(zipfile&& other) noexcept : zip_fd_(other.zip_fd_) { other.zip_fd_ = nullptr; }
    zipfile& operator=(zipfile&& other) noexcept {
        if (&other == this) { return *this; }
        zip_fd_ = other.zip_fd_;
        other.zip_fd_ = nullptr;
        return *this;
    }

    bool valid() const noexcept { return zip_fd_ != nullptr; }
    explicit operator bool() const noexcept { return zip_fd_ != nullptr; }

    UXS_EXPORT bool open(ziparch& arch, const char* fname);
    UXS_EXPORT bool open(ziparch& arch, const wchar_t* fname);
    UXS_EXPORT void close() noexcept;

    UXS_EXPORT int read(void* buf, std::size_t sz, std::size_t& n_read) override;
    int write(const void* /*data*/, std::size_t /*sz*/, std::size_t& /*n_written*/) override { return -1; }
    int flush() override { return -1; }

 private:
    void* zip_fd_ = nullptr;
};

}  // namespace uxs
