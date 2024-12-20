// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#pragma once
#include "CoreMinimal.h"
#include <functional>
#include <string.h>
#include "Layout/Margin.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Layout/Anchors.h"
#include "Fonts/SlateFontInfo.h"
#include "Engine/World.h"
#include "lua.h"

namespace NS_SLUA {

    template<typename T>
    struct AutoDeleteArray {
        AutoDeleteArray(T* p):ptr(p) {}
        ~AutoDeleteArray() { delete[] ptr; }
        T* ptr;
    };

    struct Defer {
        Defer(const std::function<void()>& f):func(f) {}
        ~Defer() { func(); }
        const std::function<void()>& func;
    };

    template<typename T>
    struct remove_cr
    {
        typedef T type;
    };

    template<typename T>
    struct remove_cr<const T&>
    {
        typedef typename remove_cr<T>::type type;
    };

    template<typename T>
    struct remove_cr<T&>
    {
        typedef typename remove_cr<T>::type type;
    };

    template<typename T>
    struct remove_cr<T&&>
    {
        typedef typename remove_cr<T>::type type;
    };

    template<class T>
    struct remove_ptr_const
    {
        typedef T type;
    };

    template<class T>
    struct remove_ptr_const<const T*>
    {
        typedef T* type;
    };

    template<class T>
    bool typeMatched(int luatype) {
        if (std::is_same<T, int32>::value
            || std::is_same<T, uint32>::value
            || std::is_same<T, int64>::value
            || std::is_same<T, uint64>::value
            || std::is_same<T, int16>::value
            || std::is_same<T, uint16>::value
            || std::is_same<T, int8>::value
            || std::is_same<T, uint8>::value
            || std::is_same<T, double>::value
            || std::is_same<T, float>::value
            || std::is_enum<T>::value)
            return luatype == LUA_TNUMBER;
        else if (std::is_same<T, bool>::value)
            return luatype == LUA_TBOOLEAN;
        else if (std::is_same<T, const char*>::value
            || std::is_same<T, FString>::value
            || std::is_same<T, FText>::value)
            return luatype == LUA_TSTRING;
        else if (std::is_base_of<T, UObject>::value
            || std::is_same<T, UObject>::value)
            return luatype == LUA_TUSERDATA;
        else if (std::is_same<T, void*>::value)
            return luatype == LUA_TLIGHTUSERDATA || luatype == LUA_TUSERDATA; 
        else
            return luatype != LUA_TNIL && luatype != LUA_TNONE;
    }

    // modified FString::Split function to return left if no InS to search
    static bool strSplit(const FString& S, const FString& InS, FString* LeftS, FString* RightS, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
        ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        if (S.IsEmpty()) return false;


        int32 InPos = S.Find(InS, SearchCase, SearchDir);

        if (InPos < 0) {
            *LeftS = S;
            *RightS = "";
            return true;
        }

        if (LeftS) { *LeftS = S.Left(InPos); }
        if (RightS) { *RightS = S.Mid(InPos + InS.Len()); }

        return true;
    }

    // why not use std::string?
    // std::string in unreal4 will caused crash
    // why not use FString
    // FString store wchar_t, we only need char
    struct SLUA_UNREAL_API SimpleString {
        static uint32 Seed;
        TArray<char> data;

        void append(const char* str) {
            if (str == nullptr)
                return;

            if (data.Num() > 0 && data[data.Num() - 1] == 0)
                data.RemoveAt(data.Num() - 1);

            data.Append(str, strlen(str) + 1);
        }
        void append(const SimpleString& str) {
            append(str.c_str());
        }
        const char* c_str() const {
            return data.GetData();
        }
        void clear() {
            data.Empty();
        }

        SimpleString()
        {
            data.Add(0);
        }

        SimpleString(const char* str)
        {
            append(str);
        }

        friend int32 GetTypeHash(const SimpleString& simpleString)
        {
            auto &data = simpleString.data;
            uint32 Len = data.Num() - 1;
		    uint32 H = Seed ^ Len;
		    uint32 Step = (Len >> 2) + 1;
		    for (; Len >= Step; Len -= Step)
		    {
		        H ^= (H << 5) + (H >> 2) + TChar<ANSICHAR>::ToUpper(data[Len - 1]);
		    }

            return H;
        }

        FORCEINLINE bool operator == (const SimpleString& Other) const
        {
            if (data.Num() != Other.data.Num())
            {
                return false;
            }
            return FPlatformString::Stricmp(data.GetData(), Other.data.GetData()) == 0;
        }
    };

    template<typename T, bool isUObject = std::is_base_of<UObject, T>::value>
    struct TypeName {
        static SimpleString value();
    };

    template<typename T>
    struct TypeName<T, true> {
        static SimpleString value() {
            return "UObject";
        }
    };

    template<typename T>
    struct TypeName<const T, false> {
        static SimpleString value() {
            return TypeName<T>::value();
        }
    };

    template<typename T>
    struct TypeName<const T*, false> {
        static SimpleString value() {
            return TypeName<T>::value();
        }
    };

    template<typename T>
    struct TypeName<T*, false> {
        static SimpleString value() {
            return TypeName<T>::value();
        }
    };

#define DefTypeName(T) \
    template<> \
    struct TypeName<T, false> { \
        static SimpleString value() { \
            return SimpleString(#T);\
        }\
    };\

#define DefTypeNameWithName(T,TN) \
    template<> \
    struct TypeName<T, false> { \
        static SimpleString value() { \
            return SimpleString(#TN);\
        }\
    };\

    DefTypeName(void);
    DefTypeName(int32);
    DefTypeName(uint32);
    DefTypeName(int16);
    DefTypeName(uint16);
    DefTypeName(int8);
    DefTypeName(uint8);
    DefTypeName(float);
    DefTypeName(double);
    DefTypeName(FString);
    DefTypeName(bool);
    DefTypeName(char);
    DefTypeName(lua_State);
    // add your custom Type-Maped here
    DefTypeName(FHitResult);
    DefTypeName(FActorSpawnParameters);
    DefTypeName(FSlateFontInfo);
    DefTypeName(FSlateBrush);
    DefTypeName(FMargin);
    DefTypeName(FGeometry);
    DefTypeName(FSlateColor);
    DefTypeName(FAnchors);
    DefTypeName(FActorComponentTickFunction);
    
    template<typename T,ESPMode mode>
    struct TypeName<TSharedPtr<T, mode>, false> {
        static SimpleString value() {
            SimpleString str;
            str.append("TSharedPtr<");
            str.append(TypeName<T>::value());
            str.append(">");
            return str;
        }
    };

    template<typename T>
    struct TypeName<TArray<T>, false> {
        static SimpleString value() {
            SimpleString str;
            str.append("TArray<");
            str.append(TypeName<T>::value());
            str.append(">");
            return str;
        }
    };
    template<typename K,typename V>
    struct TypeName<TMap<K,V>, false> {
        static SimpleString value() {
            SimpleString str;
            str.append("TMap<");
            str.append(TypeName<K>::value());
            str.append(",");
            str.append(TypeName<V>::value());
            str.append(">");
            return str;
        }
    };

    template<typename T>
    struct TypeName<TSet<T>, false> {
        static SimpleString value() {
            SimpleString str;
            str.append("TSet<");
            str.append(TypeName<T>::value());
            str.append(">");
            return str;
        }
    };

    // 基础数学类型特化
    template<>
    struct TypeName<UE::Math::TRotator<double>, 0> {
        static SimpleString value() {
            return SimpleString("Rotator");
        }
    };

    template<>
    struct TypeName<UE::Math::TVector<double>, 0> {
        static SimpleString value() {
            return SimpleString("Vector");
        }
    };

    template<>
    struct TypeName<UE::Math::TQuat<double>, 0> {
        static SimpleString value() {
            return SimpleString("Quat");
        }
    };

    // 基础UE类型特化
    template<>
    struct TypeName<FLinearColor, 0> {
        static SimpleString value() {
            return SimpleString("LinearColor");
        }
    };

    template<>
    struct TypeName<FColor, 0> {
        static SimpleString value() {
            return SimpleString("Color");
        }
    };

    // 范围类型特化
    template<>
    struct TypeName<FFloatRange, 0> {
        static SimpleString value() {
            return SimpleString("FloatRange");
        }
    };

    template<>
    struct TypeName<FDoubleRange, 0> {
        static SimpleString value() {
            return SimpleString("DoubleRange");
        }
    };

    // 1. UE Math Types
    template<>
    struct TypeName<UE::Math::TIntPoint<int>, 0> {
        static SimpleString value() { return "IntPoint"; }
    };

    template<>
    struct TypeName<UE::Math::TIntVector3<int>, 0> {
        static SimpleString value() { return "IntVector3"; }
    };

    template<>
    struct TypeName<UE::Math::TIntVector4<int>, 0> {
        static SimpleString value() { return "IntVector4"; }
    };

    template<>
    struct TypeName<UE::Math::TVector2<double>, 0> {
        static SimpleString value() { return "Vector2D"; }
    };

    template<>
    struct TypeName<UE::Math::TVector4<double>, 0> {
        static SimpleString value() { return "Vector4"; }
    };

    // 2. Basic UE Types
    template<>
    struct TypeName<FRandomStream, 0> {
        static SimpleString value() { return "RandomStream"; }
    };

    template<>
    struct TypeName<FGuid, 0> {
        static SimpleString value() { return "Guid"; }
    };

    template<>
    struct TypeName<FFallbackStruct, 0> {
        static SimpleString value() { return "FallbackStruct"; }
    };

    template<>
    struct TypeName<FDateTime, 0> {
        static SimpleString value() { return "DateTime"; }
    };

    // 3. Range & Interval Types
    template<>
    struct TypeName<FFloatRangeBound, 0> {
        static SimpleString value() { return "FloatRangeBound"; }
    };

    template<>
    struct TypeName<FDoubleRangeBound, 0> {
        static SimpleString value() { return "DoubleRangeBound"; }
    };

    template<>
    struct TypeName<FInt32RangeBound, 0> {
        static SimpleString value() { return "Int32RangeBound"; }
    };

    template<>
    struct TypeName<FInt32Range, 0> {
        static SimpleString value() { return "Int32Range"; }
    };

    // 4. Interp Curve Types
    template<>
    struct TypeName<FInterpCurvePoint<float>, 0> {
        static SimpleString value() { return "InterpCurvePointFloat"; }
    };

    template<>
    struct TypeName<FInterpCurvePoint<UE::Math::TVector2<double>>, 0> {
        static SimpleString value() { return "InterpCurvePointVector2D"; }
    };

    // 5. Asset Types
    template<>
    struct TypeName<FSoftObjectPath, 0> {
        static SimpleString value() { return "SoftObjectPath"; }
    };

    template<>
    struct TypeName<FSoftClassPath, 0> {
        static SimpleString value() { return "SoftClassPath"; }
    };

    template<>
    struct TypeName<FPrimaryAssetType, 0> {
        static SimpleString value() { return "PrimaryAssetType"; }
    };

// 插值曲线特化
    template<>
    struct TypeName<FInterpCurvePoint<UE::Math::TVector<double>>, 0> {
        static SimpleString value() { return "InterpCurvePointVector"; }
    };

    template<>
    struct TypeName<FInterpCurvePoint<UE::Math::TQuat<double>>, 0> {
        static SimpleString value() { return "InterpCurvePointQuat"; }
    };

    template<>
    struct TypeName<FInterpCurvePoint<FTwoVectors>, 0> {
        static SimpleString value() { return "InterpCurvePointTwoVectors"; }
    };

    template<>
    struct TypeName<FInterpCurvePoint<FLinearColor>, 0> {
        static SimpleString value() { return "InterpCurvePointLinearColor"; }
    };

    // 区间类型特化
    template<>
    struct TypeName<FFloatInterval, 0> {
        static SimpleString value() { return "FloatInterval"; }
    };

    template<>
    struct TypeName<FDoubleInterval, 0> {
        static SimpleString value() { return "DoubleInterval"; }
    };

    template<>
    struct TypeName<FInt32Interval, 0> {
        static SimpleString value() { return "Int32Interval"; }
    };

    // 时间相关类型特化
    template<>
    struct TypeName<FFrameNumber, 0> {
        static SimpleString value() { return "FrameNumber"; }
    };

    template<>
    struct TypeName<FFrameTime, 0> {
        static SimpleString value() { return "FrameTime"; }
    };

    // 资产相关类型特化
    template<>
    struct TypeName<FPrimaryAssetId, 0> {
        static SimpleString value() { return "PrimaryAssetId"; }
    };

    template<>
    struct TypeName<FTopLevelAssetPath, 0> {
        static SimpleString value() { return "TopLevelAssetPath"; }
    };

    // UE数学类型特化
    template<>
    struct TypeName<UE::Math::TPlane<double>, 0> {
        static SimpleString value() { return "Plane"; }
    };

    template<>
    struct TypeName<UE::Math::TTransform<double>, 0> {
        static SimpleString value() { return "Transform"; }
    };

    template<>
    struct TypeName<UE::Math::TMatrix<double>, 0> {
        static SimpleString value() { return "Matrix"; }
    };

    template<>
    struct TypeName<UE::Math::TBox2<double>, 0> {
        static SimpleString value() { return "Box2D"; }
    };

    template<>
    struct TypeName<UE::Math::TRay<double>, 0> {
        static SimpleString value() { return "Ray"; }
    };

    template<>
    struct TypeName<UE::Math::TSphere<double>, 0> {
        static SimpleString value() { return "Sphere"; }
    };
    

    template<class R, class ...ARGS>
    struct MakeGeneircTypeName {
        static void get(SimpleString& output, const char* delimiter) {
            MakeGeneircTypeName<R>::get(output, delimiter);
            MakeGeneircTypeName<ARGS...>::get(output, delimiter);
        }
    };

    template<class R>
    struct MakeGeneircTypeName<R> {
        static void get(SimpleString& output, const char* delimiter) {
            output.append(TypeName<typename remove_cr<R>::type>::value());
            output.append(delimiter);
        }
    };

    // return true if T is UObject or is base of UObject
    template<class T>
    struct IsUObject {
        enum { value = std::is_base_of<UObject, T>::value || std::is_same<UObject, T>::value };
    };

    template<class T>
    struct IsUObject<T*> {
        enum { value = IsUObject<T>::value };
    };

    template<class T>
    struct IsUObject<const T*> {
        enum { value = IsUObject<T>::value };
    };
    
    // lua long string 
    // you can call push(L,{str,len}) to push LuaLString
    struct LuaLString {
        const char* buf;
        size_t len;
    };

    // SFINAE test class has a specified member function
    template <typename T>
    class Has_LUA_typename
    {
    private:
        typedef char WithType;
        typedef int WithoutType;

        template <typename C> 
        static WithType test(decltype(&C::LUA_typename));
        template <typename C> 
        static WithoutType test(...);

    public:
        enum { value = sizeof(test<T>(0)) == sizeof(WithType) };
    };

    FString SLUA_UNREAL_API getUObjName(UObject* obj);
    bool SLUA_UNREAL_API isUnrealStruct(const char* tn, UScriptStruct** out);


    int64_t SLUA_UNREAL_API getTime();
}
