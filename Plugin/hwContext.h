﻿#ifndef hwContext_h
#define hwContext_h

struct hwShaderData
{
    hwHShader handle;
    int ref_count;
    ID3D11PixelShader *shader;
    std::string path;

    hwShaderData() : handle(hwNullHandle), ref_count(0), shader(nullptr) {}
    void invalidate() { ref_count = 0; shader = nullptr; path.clear(); }
    operator bool() const { return shader != nullptr; }
};

struct hwAssetData
{
    hwHAsset handle;
    int ref_count;
    hwAssetID aid;
    std::string path;
    hwConversionSettings settings;
    std::vector<hwMatrix> m_inv_bindpose;

    hwAssetData() : handle(hwNullHandle), aid(hwNullAssetID), ref_count(0) {}
    void invalidate() { ref_count = 0; aid = hwNullAssetID; path.clear(); }
    operator bool() const { return aid != hwNullAssetID; }
};

struct hwInstanceData
{
    hwHInstance handle;
    hwInstanceID iid;
    hwHAsset hasset;
    bool cast_shadow;
    bool receive_shadow;

    hwInstanceData() : handle(hwNullHandle), iid(hwNullInstanceID), hasset(hwNullHandle), cast_shadow(false), receive_shadow(false) {}
    void invalidate() { iid = hwNullInstanceID; hasset = hwNullAssetID; cast_shadow = false; receive_shadow = false; }
    operator bool() const { return iid != hwNullInstanceID; }
};

enum hwELightType
{
    hwELightType_Directional,
    hwELightType_Point,
};

struct hwLightData
{
    hwELightType type; int pad[3];
    hwFloat3 position; // direction if directional light, position if point light
    float range;
    hwFloat4 color;

    hwLightData()
        : type(hwELightType_Directional)
        , position({ 0.0f, 0.0f, 0.0f })
        , range(0.0f)
        , color({ 1.0f, 1.0f, 1.0f, 1.0 })
    {}
};

struct hwConstantBuffer
{
    int num_lights; int pad0[3];
    hwLightData lights[hwMaxLights];
    GFSDK_HairShaderConstantBuffer hw;

    hwConstantBuffer() : num_lights(0) {}
};



class hwContext
{
public:
    // メインスレッドと Unity のレンダリングスレッドは別である可能性があるため、
    // 外に見せる描画系関数はコマンドを積むだけにして、UnityRenderEvent() でそれを flush するという形式をとる。
    enum CommandID
    {
        CID_SetViewProjection,
        CID_SetRenderTarget,
        CID_SetShader,
        CID_SetLights,
        CID_Render,
        CID_RenderShadow,
    };
    struct DrawCommandVP
    {
        CommandID command;
        float fov;
        hwMatrix view;
        hwMatrix proj;
    };
    struct DrawCommandRT
    {
        CommandID command;
        hwTexture *framebuffer;
        hwTexture *depthbuffer;
    };
    struct DrawCommandL
    {
        CommandID command;
        int num_lights;
        hwLightData lights[hwMaxLights];
    };
    struct DrawCommand
    {
        CommandID command;
        int arg;
    };



public:
    hwContext();
    ~hwContext();
    bool valid() const;

    bool initialize(const char *path_to_dll, hwDevice *d3d_device);
    void finalize();
    void move(hwContext &from);

    hwHShader       shaderLoadFromFile(const std::string &path);
    void            shaderRelease(hwHShader sid);
    void            shaderReload(hwHShader sid);

    hwHAsset        assetLoadFromFile(const std::string &path, const hwConversionSettings &conv);
    void            assetRelease(hwHAsset aid);
    void            assetReload(hwHAsset aid);
    int             assetGetNumBones(hwHAsset aid) const;
    const char*     assetGetBoneName(hwHAsset aid, int nth) const;
    void            assetGetBoneIndices(hwHAsset aid, hwFloat4 &o_indices) const;
    void            assetGetBoneWeights(hwHAsset aid, hwFloat4 &o_waits) const;
    void            assetGetDefaultDescriptor(hwHAsset aid, hwHairDescriptor &o_desc) const;

    hwHInstance     instanceCreate(hwHAsset aid);
    void            instanceRelease(hwHInstance iid);
    void            instanceGetDescriptor(hwHInstance iid, hwHairDescriptor &desc) const;
    void            instanceSetDescriptor(hwHInstance iid, const hwHairDescriptor &desc);
    void            instanceSetTexture(hwHInstance iid, hwTextureType type, hwTexture *tex);
    void            instanceUpdateSkinningMatrices(hwHInstance iid, int num_matrices, hwMatrix *matrices);

    void setViewProjection(const hwMatrix &view, const hwMatrix &proj, float fov);
    void setRenderTarget(hwTexture *framebuffer, hwTexture *depthbuffer);
    void setShader(hwHShader sid);
    void setLights(int num_lights, const hwLightData *lights);
    void render(hwHInstance iid);
    void renderShadow(hwHInstance iid);
    void flush();
    void stepSimulation(float dt);

private:
    hwShaderData&   newShaderData();
    hwAssetData&    newAssetData();
    hwInstanceData& newInstanceData();

    template<class T> void pushDrawCommand(const T &c);
    void setViewProjectionImpl(const hwMatrix &view, const hwMatrix &proj, float fov);
    void setRenderTargetImpl(hwTexture *framebuffer, hwTexture *depthbuffer);
    void setShaderImpl(hwHShader sid);
    void setLightsImpl(int num_lights, const hwLightData *lights);
    void renderImpl(hwHInstance iid);
    void renderShadowImpl(hwHInstance iid);
    hwSRV* getSRV(hwTexture *tex);
    hwRTV* getRTV(hwTexture *tex);

private:
    typedef std::vector<hwShaderData>       ShaderCont;
    typedef std::vector<hwAssetData>        AssetCont;
    typedef std::vector<hwInstanceData>     InstanceCont;
    typedef std::map<hwTexture*, hwSRV*>    SRVTable;
    typedef std::map<hwTexture*, hwRTV*>    RTVTable;
    typedef std::vector<char>               DrawCommands;

    ID3D11Device            *m_d3ddev;
    ID3D11DeviceContext     *m_d3dctx;
    hwSDK                   *m_sdk;
    ShaderCont              m_shaders;
    AssetCont               m_assets;
    InstanceCont            m_instances;
    SRVTable                m_srvtable;
    RTVTable                m_rtvtable;
    DrawCommands            m_commands;

    ID3D11DepthStencilState *m_rs_enable_depth;
    ID3D11Buffer            *m_rs_constant_buffer;

    hwConstantBuffer        m_cb;
};

#endif // hwContext_h
