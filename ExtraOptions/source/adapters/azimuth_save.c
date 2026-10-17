#include "azimuth_host.h"
#include "azimuth_markers.h"
#include "logging.h"

static const unsigned char skip_hash_rejection[] = {0x90, 0x90};
static const unsigned char skip_size_rejection[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
static const HostCodePatch revision_patches[] = {
    {0x4b, sizeof(skip_size_rejection), skip_size_rejection}};
static const HostCodePatch reader_patches[] = {
    {0x13, sizeof(skip_size_rejection), skip_size_rejection},
    {0x378, sizeof(skip_hash_rejection), skip_hash_rejection}};
static const unsigned char revision_original_hash[32] = {
    0x2a, 0x9c, 0x94, 0xad, 0x59, 0x38, 0xfe, 0xcc, 0x3f, 0x30, 0x8d, 0x89, 0xca, 0x6f, 0x0a, 0x5e,
    0xf5, 0xe1, 0x9a, 0x3d, 0xf5, 0x84, 0xc2, 0x5d, 0x8b, 0x8d, 0x28, 0x6d, 0xb6, 0x10, 0xe6, 0xb4};
static const unsigned char revision_patched_hash[32] = {
    0x3c, 0xcb, 0xcc, 0xf0, 0x8f, 0xf4, 0xb0, 0x16, 0xca, 0xed, 0xd9, 0xa4, 0x15, 0x2c, 0x82, 0x0b,
    0x78, 0xd7, 0x0d, 0x66, 0xf8, 0xf1, 0xb0, 0x2d, 0xff, 0xae, 0x59, 0x52, 0xd7, 0xb3, 0x7c, 0x12};
static const unsigned char reader_original_hash[32] = {
    0x60, 0xbf, 0x1f, 0x9a, 0x1f, 0xae, 0x41, 0xe3, 0x3a, 0xb5, 0x56, 0x57, 0xd7, 0x34, 0xef, 0x36,
    0x8b, 0xd9, 0x7d, 0x43, 0x56, 0x12, 0x8c, 0x23, 0xb2, 0x83, 0x80, 0xe5, 0xde, 0x99, 0x5d, 0xa4};
static const unsigned char reader_patched_hash[32] = {
    0xfa, 0x16, 0x11, 0xf6, 0x1a, 0x42, 0xb8, 0x84, 0x13, 0x50, 0x52, 0xb0, 0x3d, 0xa4, 0xa8, 0x55,
    0xc7, 0x0d, 0x69, 0x52, 0x77, 0x6b, 0x3c, 0x0e, 0xde, 0x41, 0x68, 0x02, 0x04, 0xd0, 0x73, 0xc8};
static const HostPatchSet validation_patch_sets[] = {
    {0x182c49, 0x51, revision_original_hash, revision_patched_hash, revision_patches,
     ARRAY_COUNT(revision_patches)},
    {0x183610, 0x3e5, reader_original_hash, reader_patched_hash, reader_patches,
     ARRAY_COUNT(reader_patches)}};

static int apply_save(unsigned char *base) {
    return azimuth_host_build(base) == AZIMUTH_0910
               ? azimuth_patch_host_codes(base, validation_patch_sets,
                                          ARRAY_COUNT(validation_patch_sets))
               : 0;
}

void azimuth_apply_save_compatibility(int disable_checksums) {
    if (!disable_checksums) {
        log_append("AZIMUTH: native save checksum and file-size verification "
                   "enabled.\r\n");
        return;
    }
    int result = apply_save((unsigned char *)GetModuleHandleW(NULL));
    if (result > 0) {
        log_append("AZIMUTH: save checksum and file-size comparisons disabled; "
                   "marker format and normal save writing retained.\r\n");
        azimuth_repair_missing_markers((unsigned char *)GetModuleHandleW(NULL));
    } else if (result < 0) {
        log_append("AZIMUTH: save checksum override could not be applied.\r\n");
    } else {
        log_append("AZIMUTH: save checksum override skipped; "
                   "host is not a supported inspected save-validation build.\r\n");
    }
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) int Fh2TestAzimuthSaveCompatibility(void *image, int disable_checksums) {
    return disable_checksums ? apply_save(image) : 0;
}
#endif
