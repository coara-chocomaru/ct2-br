$(call inherit-product-if-exists, vendor/sts/a03br/a03br-vendor.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product_launched_with_l_mr1.mk)
PRODUCT_FIRST_API_LEVEL := 22
PRODUCT_CHARACTERISTICS := tablet
TARGET_IS_TABLET := true
TARGET_NO_TELEPHONY := true
TARGET_HAS_NO_RADIO := true
LOCAL_PATH := device/sts/a03br
PRODUCT_BUILD_RECOVERY_IMAGE := true
PRODUCT_BUILD_SYSTEM_IMAGE := true
PRODUCT_BUILD_BOOT_IMAGE := true
PRODUCT_BUILD_VENDOR_IMAGE := false

# Product characteristics
# PRODUCT_PACKAGE_OVERLAYS += \
    $(LOCAL_PATH)/overlay \
    $(LOCAL_PATH)/overlay-lineage

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.adb.secure=0 \
    ro.secure=0 \
    ro.debuggable=1 \
    ro.boot.wificountrycode=JP \
    security.perf_harden=1 \
    ro.config.donot_nosim=true \
    persist.radio.no_wait_for_card=1 \
    ro.radio.noril=1 \
    persist.bandwidth.enable=0 \
    ro.zygote=zygote64_32

ADDITIONAL_DEFAULT_PROPERTIES += \
    ro.adb.secure=0 \
    ro.secure=0 \
    ro.debuggable=1 \
    ro.boot.wificountrycode=JP \
    persist.bandwidth.enable=0 \
    security.perf_harden=1 \
    ro.oem_unlock_supported=0 \
    ro.com.android.mobiledata=false \
    ro.allow.mock.location=0 \
    ro.zygote=zygote64_32 \
    ro.mount.fs=EXT4

PRODUCT_PROPERTY_OVERRIDES += \
    ro.build.selinux=0 \
    ro.telephony.disable=true \
    ro.kernel.android.checkjni=0 \
    ro.carrier=wifi-only \
    persist.sys.country=JP \
    ro.radio.noril=1 \
    config.disable_telephony=true \
    keyguard.no_require_sim=true \
    dalvik.vm.dex2oat-filter=speed-profile \
    dalvik.vm.image-dex2oat-filter=speed-profile \
    dalvik.vm.dex2oat-threads=4 \
    dalvik.vm.dex2oat-flags=--no-watch-dog

ADDITIONAL_BUILD_PROPERTIES += \
    ro.build.selinux=0 \
    persist.radio.noril=1 \
    ro.setupwizard.enable_bypass=1 \
    ro.setupwizard.enterprise_mode=0 \
    ro.setupwizard.mode=OPTIONAL \
    ro.setupwizard.gservices_delay=-1 \
    ro.setupwizard.network_required=false \
    ro.setupwizard.rotation_locked=true \
    ro.carrier=wifi-only \
    persist.bandwidth.enable=0 \
    dalvik.vm.heapstartsize=4m \
    dalvik.vm.heapgrowthlimit=192m \
    dalvik.vm.heapsize=384m \
    dalvik.vm.heaptargetutilization=0.75 \
    dalvik.vm.heapminfree=2m \
    dalvik.vm.heapmaxfree=4m \
    dalvik.vm.checkjni=false \
    ro.hwui.texture_cache_size=40 \
    ro.hwui.layer_cache_size=24 \
    ro.hwui.path_cache_size=12 \
    ro.hwui.gradient_cache_size=1 \
    ro.hwui.drop_shadow_cache_size=4 \
    ro.hwui.r_buffer_cache_size=4 \
    ro.hwui.texture_cache_flushrate=0.40 \
    ro.hwui.text_small_cache_width=1024 \
    ro.hwui.text_small_cache_height=512 \
    ro.hwui.text_large_cache_width=1280 \
    ro.hwui.text_large_cache_height=512 \
    media.sf.omx-plugin=libffmpeg_omx.so \
    media.sf.extractor-plugin=libffmpeg_extractor.so \
    ro.storage_manager.enabled=true \
    media.stagefright.legacyencoder=true \
    media.stagefright.less-secure=true \
    ro.telephony.disable=true \
    ro.config.ringtone=Zen.ogg \
    ro.config.alarm_alert=Flow.ogg \
    ro.config.notification_sound=Chime.ogg \
    ro.btstack=blueangel \
    ro.sf.hwrotation=270 \
    fmradio.driver.enable=0 \
    ro.mtk_support_mp2_playback=1 \
    ro.mtk_audio_alac_support=1 \
    ro.mtk_search_db_support=1 \
    ro.mtk_dhcpv6c_wifi=1 \
    ro.have_aacencode_feature=1 \
    ro.mtk_audio_ape_support=1 \
    ro.mtk_wmv_playback_support=1 \
    ro.mtk_flight_mode_power_off_md=1 \
    ro.mtk_emmc_support=1 \
    ro.mtk_shared_sdcard=1 \
    ro.mtk_blulight_def_support=1 \
    ro.mtk_owner_sdcard_support=0 \
    ro.mtk_multi_patition=1 \
    ro.mtk_bg_power_saving_support=0 \
    ro.mtk_bg_power_saving_ui=0 \
    ro.have_aee_feature=0 \
    ro.mtk_is_tablet=1 \
    wfd.dummy.enable=0 \
    wfd.iframesize.level=0 \
    ro.mediatek.project.path=device/sts/a04br3 \
    ro.mtk_sec_video_path_support=1 \
    persist.mtk.wcn.dynamic.dump=0 \
    ro.com.android.mobiledata=false \
    ro.mtk_deinterlace_support=1 \
    ro.boot.opt_using_default=1 \
    mtk.vdec.waitkeyframeforplay=1 \
    ro.sys.sdcardfs=1 \
    ro.com.google.ime.theme_id=5 \
    persist.sys.timezone=Asia/Tokyo

PRODUCT_PACKAGES += \
    LatinIME

PRODUCT_PACKAGES += \
    libstlport

PRODUCT_PACKAGES += \
    libion

PRODUCT_PACKAGES += \
    com.android.future.usb.accessory

PRODUCT_PACKAGES += \
    e2fsck \
    fsck.f2fs \
    mkfs.f2fs \
    make_ext4fs

PRODUCT_PACKAGES += \
    fstab.mt8167 \
    init.mt8167.rc \
    init.mt8167.usb.rc \
    init.usb.rc \
    init.usb.configfs.rc \
    init.recovery.mt8167.rc \
    init.connectivity.rc \
    ueventd.mt8167.rc \
    ueventd.rc \
    init.modem.rc \
    init.zygote32.rc \
    init.zygote64_32.rc \
    init.cm.rc \
    init.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/etc/init.mt8167.rc:root/init.mt8167.rc \
    $(LOCAL_PATH)/rootdir/etc/init.mt8167.usb.rc:root/init.mt8167.usb.rc \
    $(LOCAL_PATH)/rootdir/etc/fstab.mt8167:root/fstab.mt8167 \
    $(LOCAL_PATH)/rootdir/etc/init.recovery.mt8167.rc:root/init.recovery.mt8167.rc \
    $(LOCAL_PATH)/rootdir/etc/init.connectivity.rc:root/init.connectivity.rc \
    $(LOCAL_PATH)/rootdir/etc/ueventd.mt8167.rc:root/ueventd.mt8167.rc \
    $(LOCAL_PATH)/rootdir/etc/init.usb.rc:root/init.usb.rc \
    $(LOCAL_PATH)/rootdir/etc/init.usb.configfs.rc:root/init.usb.configfs.rc \
    $(LOCAL_PATH)/rootdir/etc/ueventd.rc:root/ueventd.rc \
    $(LOCAL_PATH)/rootdir/etc/init.modem.rc:root/init.modem.rc \
    $(LOCAL_PATH)/rootdir/etc/init.zygote32.rc:root/init.zygote32.rc \
    $(LOCAL_PATH)/rootdir/etc/init.zygote64_32.rc:root/init.zygote64_32.rc \
    $(LOCAL_PATH)/rootdir/etc/init.cm.rc:root/init.cm.rc \
    $(LOCAL_PATH)/rootdir/etc/init.rc:root/init.rc

include device/sts/a04br3/media.mk
include device/sts/a04br3/wifi.mk
include device/sts/a04br3/permissions.mk
