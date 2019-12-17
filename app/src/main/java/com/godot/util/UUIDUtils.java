package com.godot.util;

import androidx.annotation.Keep;

import java.util.UUID;

/**
 * @Desc 说明
 * @Author Godot
 * @Date 2019-12-17
 * @Version 1.0
 * @Mail 809373383@qq.com
 */
@Keep
public class UUIDUtils {

    private UUIDUtils() {
    }

    public static ClassLoader getClassLoader() {
        return UUIDUtils.class.getClassLoader();
    }

    public static String getUUID() {
        return UUID.randomUUID().toString();
    }
}
