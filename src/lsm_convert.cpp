/*
 * Copyright (C) 2011-2013 Red Hat, Inc.
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: tasleson
 */

#include "lsm_convert.hpp"
#include "libstoragemgmt/libstoragemgmt_accessgroups.h"
#include <libstoragemgmt/libstoragemgmt_blockrange.h>
#include <libstoragemgmt/libstoragemgmt_nfsexport.h>

static bool isExpectedObject(Value &obj, std::string class_name)
{
    if (obj.valueType() == Value::object_t) {
        std::map<std::string, Value> i = obj.asObject();
        std::map<std::string, Value>::iterator iter = i.find("class");
        if (iter != i.end() && iter->second.asString() == class_name) {
            return true;
        }
    }
    return false;
}

lsmVolume *valueToVolume(Value &vol)
{
    lsmVolume *rc = NULL;

    if (isExpectedObject(vol, "Volume")) {
        std::map<std::string, Value> v = vol.asObject();
        rc = lsmVolumeRecordAlloc(
            v["id"].asString().c_str(),
            v["name"].asString().c_str(),
            v["vpd83"].asString().c_str(),
            v["block_size"].asUint64_t(),
            v["num_of_blocks"].asUint64_t(),
            v["status"].asUint32_t(),
            v["system_id"].asString().c_str(),
            v["pool_id"].asString().c_str());
    }

    return rc;
}

Value volumeToValue(lsmVolume *vol)
{
    if( LSM_IS_VOL(vol) ) {
        std::map<std::string, Value> v;
        v["class"] = Value("Volume");
        v["id"] = Value(vol->id);
        v["name"] = Value(vol->name);
        v["vpd83"] = Value(vol->vpd83);
        v["block_size"] = Value(vol->blockSize);
        v["num_of_blocks"] = Value(vol->numberOfBlocks);
        v["status"] = Value(vol->status);
        v["system_id"] = Value(vol->system_id);
        v["pool_id"] = Value(vol->pool_id);
        return Value(v);
    }
    return Value();
}

lsmInitiator *valueToInitiator(Value &init)
{
    lsmInitiator *rc = NULL;

    if (isExpectedObject(init, "Initiator")) {
        std::map<std::string, Value> i = init.asObject();
        rc = lsmInitiatorRecordAlloc(
            (lsmInitiatorType) i["type"].asInt32_t(),
            i["id"].asString().c_str(),
            i["name"].asString().c_str()
            );
    }
    return rc;

}

Value initiatorToValue(lsmInitiator *init)
{
    if( LSM_IS_INIT(init) ) {
        std::map<std::string, Value> i;
        i["class"] = Value("Initiator");
        i["type"] = Value((int32_t) init->idType);
        i["id"] = Value(init->id);
        i["name"] = Value(init->name);
        return Value(i);
    }
    return Value();
}

lsmPool *valueToPool(Value &pool)
{
    lsmPool *rc = NULL;

    if (isExpectedObject(pool, "Pool")) {
        std::map<std::string, Value> i = pool.asObject();
        rc = lsmPoolRecordAlloc(
            i["id"].asString().c_str(),
            i["name"].asString().c_str(),
            i["total_space"].asUint64_t(),
            i["free_space"].asUint64_t(),
            i["system_id"].asString().c_str());
    }
    return rc;
}

Value poolToValue(lsmPool *pool)
{
    if( LSM_IS_POOL(pool) ) {
        std::map<std::string, Value> p;
        p["class"] = Value("Pool");
        p["id"] = Value(pool->id);
        p["name"] = Value(pool->name);
        p["total_space"] = Value(pool->totalSpace);
        p["free_space"] = Value(pool->freeSpace);
        p["system_id"] = Value(pool->system_id);
        return Value(p);
    }
    return Value();
}

lsmSystem *valueToSystem(Value &system)
{
    lsmSystem *rc = NULL;
    if (isExpectedObject(system, "System")) {
        std::map<std::string, Value> i = system.asObject();
        rc = lsmSystemRecordAlloc(  i["id"].asString().c_str(),
                                    i["name"].asString().c_str(),
                                    i["status"].asUint32_t());
    }
    return rc;
}

Value systemToValue(lsmSystem *system)
{
    if( LSM_IS_SYSTEM(system)) {
        std::map<std::string, Value> s;
        s["class"] = Value("System");
        s["id"] = Value(system->id);
        s["name"] = Value(system->name);
        s["status"] = Value(system->status);
        return Value(s);
    }
    return Value();
}

lsmStringList *valueToStringList(Value &v)
{
    lsmStringList *il = NULL;

    if( Value::array_t == v.valueType() ) {
        std::vector<Value> vl = v.asArray();
        uint32_t size = vl.size();
        il = lsmStringListAlloc(size);

        if( il ) {
            for( uint32_t i = 0; i < size; ++i ) {
                if(LSM_ERR_OK != lsmStringListSetElem(il, i, vl[i].asC_str())){
                    lsmStringListFree(il);
                    il = NULL;
                    break;
                }
            }
        }
    }
    return il;
}

Value stringListToValue( lsmStringList *sl)
{
    std::vector<Value> rc;
    if( LSM_IS_STRING_LIST(sl) ) {
        uint32_t size = lsmStringListSize(sl);

        for(uint32_t i = 0; i < size; ++i ) {
            rc.push_back(Value(lsmStringListGetElem(sl, i)));
        }
    }
    return Value(rc);
}

lsmAccessGroup *valueToAccessGroup( Value &group )
{
    lsmStringList *il = NULL;
    lsmAccessGroup *ag = NULL;

    if( isExpectedObject(group, "AccessGroup")) {
        std::map<std::string, Value> vAg = group.asObject();

        il = valueToStringList(vAg["initiators"]);

        if( il ) {
            ag = lsmAccessGroupRecordAlloc(vAg["id"].asString().c_str(),
                                        vAg["name"].asString().c_str(),
                                        il,
                                        vAg["system_id"].asString().c_str());

            /* Initiator list is copied in AccessroupRecordAlloc */
            lsmStringListFree(il);
        }
    }
    return ag;
}

Value accessGroupToValue( lsmAccessGroup *group )
{
    if( LSM_IS_ACCESS_GROUP(group) ) {
        std::map<std::string, Value> ag;
        ag["class"] = Value("AccessGroup");
        ag["id"] = Value(group->id);
        ag["name"] = Value(group->name);
        ag["initiators"] = Value(stringListToValue(group->initiators));
        ag["system_id"] = Value(group->system_id);
        return Value(ag);
    }
    return Value();
}

lsmAccessGroup **valueToAccessGroupList( Value &group, uint32_t *count )
{
    lsmAccessGroup **rc = NULL;
    std::vector<Value> ag = group.asArray();

    *count = ag.size();

    if( *count ) {
        rc = lsmAccessGroupRecordAllocArray(*count);
        if( rc ) {
            uint32_t i;
            for(i = 0; i < *count; ++i ) {
                rc[i] = valueToAccessGroup(ag[i]);
                if( !rc[i] ) {
                    lsmAccessGroupRecordFreeArray(rc, i);
                    rc = NULL;
                    break;
                }
            }
        }
    }
    return rc;
}

Value accessGroupListToValue( lsmAccessGroup **group, uint32_t count)
{
    std::vector<Value> rc;

    if( group && count ) {
        uint32_t i;
        for( i = 0; i < count; ++i ) {
            rc.push_back(accessGroupToValue(group[i]));
        }
    }
    return Value(rc);
}

lsmBlockRange *valueToBlockRange(Value &br)
{
    lsmBlockRange *rc = NULL;
    if( isExpectedObject(br, "BlockRange") ) {
        std::map<std::string, Value> range = br.asObject();

        rc = lsmBlockRangeRecordAlloc(range["src_block"].asUint64_t(),
                                        range["dest_block"].asUint64_t(),
                                        range["block_count"].asUint64_t());
    }
    return rc;
}

Value blockRangeToValue(lsmBlockRange *br)
{
    if( LSM_IS_BLOCK_RANGE(br) ) {
        std::map<std::string, Value> r;
        r["class"] = Value("BlockRange");
        r["src_block"] = Value(br->source_start);
        r["dest_block"] = Value(br->dest_start);
        r["block_count"] = Value(br->block_count);
        return Value(r);
    }
    return Value();
}

lsmBlockRange **valueToBlockRangeList(Value &brl, uint32_t *count)
{
    lsmBlockRange **rc = NULL;
    std::vector<Value> r = brl.asArray();
    *count = r.size();
    if( *count ) {
        rc = lsmBlockRangeRecordAllocArray(*count);
        if( rc ) {
            for( uint32_t i = 0; i < *count; ++i ) {
                rc[i] = valueToBlockRange(r[i]);
                if( !rc[i] ) {
                    lsmBlockRangeRecordFreeArray(rc, i);
                    rc = NULL;
                    break;
                }
            }
        }
    }
    return rc;
}

Value blockRangeListToValue( lsmBlockRange **brl, uint32_t count )
{
    std::vector<Value> r;
    if( brl && count) {
        uint32_t i = 0;
        for( i = 0; i < count; ++i ) {
            r.push_back(blockRangeToValue(brl[i]));
        }
    }
    return Value(r);
}

lsmFs *valueToFs(Value &fs)
{
    lsmFs *rc = NULL;
    if( isExpectedObject(fs, "FileSystem") ) {
        std::map<std::string, Value> f = fs.asObject();
        rc = lsmFsRecordAlloc(f["id"].asString().c_str(),
                                f["name"].asString().c_str(),
                                f["total_space"].asUint64_t(),
                                f["free_space"].asUint64_t(),
                                f["pool_id"].asString().c_str(),
                                f["system_id"].asString().c_str());
    }
    return rc;
}

Value fsToValue(lsmFs *fs)
{
    if( LSM_IS_FS(fs) ) {
        std::map<std::string, Value> f;
        f["class"] = Value("FileSystem");
        f["id"] = Value(fs->id);
        f["name"] = Value(fs->name);
        f["total_space"] = Value(fs->total_space);
        f["free_space"] = Value(fs->free_space);
        f["pool_id"] = Value(fs->pool_id);
        f["system_id"] = Value(fs->system_id);
        return Value(f);
    }
    return Value();
}


lsmSs *valueToSs(Value &ss)
{
    lsmSs *rc = NULL;
    if( isExpectedObject(ss, "Snapshot") ) {
        std::map<std::string, Value> f = ss.asObject();
        rc = lsmSsRecordAlloc(f["id"].asString().c_str(),
                                f["name"].asString().c_str(),
                                f["ts"].asUint64_t());
    }
    return rc;
}

Value ssToValue(lsmSs *ss)
{
    if( LSM_IS_SS(ss) ) {
        std::map<std::string, Value> f;
        f["class"] = Value("Snapshot");
        f["id"] = Value(ss->id);
        f["name"] = Value(ss->name);
        f["ts"] = Value(ss->ts);
        return Value(f);
    }
    return Value();
}

lsmNfsExport *valueToNfsExport(Value &exp)
{
    lsmNfsExport *rc = NULL;
    if( isExpectedObject(exp, "NfsExport") ) {
        int ok = 0;
        lsmStringList *root = NULL;
        lsmStringList *rw = NULL;
        lsmStringList *ro = NULL;

        std::map<std::string, Value> i = exp.asObject();

        /* Check all the arrays for successful allocation */
        root = valueToStringList(i["root"]);
        if( root ) {
            rw = valueToStringList(i["rw"]);
            if( rw ) {
                ro = valueToStringList(i["ro"]);
                if( !ro ) {
                    lsmStringListFree(rw);
                    lsmStringListFree(root);
                    rw = NULL;
                    root = NULL;
                } else {
                    ok = 1;
                }
            } else {
                lsmStringListFree(root);
                root = NULL;
            }
        }

        if( ok ) {
            rc = lsmNfsExportRecordAlloc(
                i["id"].asC_str(),
                i["fs_id"].asC_str(),
                i["export_path"].asC_str(),
                i["auth"].asC_str(),
                root,
                rw,
                ro,
                i["anonuid"].asUint64_t(),
                i["anongid"].asUint64_t(),
                i["options"].asC_str()
                );

            lsmStringListFree(root);
            lsmStringListFree(rw);
            lsmStringListFree(ro);
        }
    }
    return rc;
}

Value nfsExportToValue(lsmNfsExport *exp)
{
    if( LSM_IS_NFS_EXPORT(exp) ) {
        std::map<std::string, Value> f;
        f["class"] = Value("NfsExport");
        f["id"] = Value(exp->id);
        f["fs_id"] = Value(exp->fs_id);
        f["export_path"] = Value(exp->export_path);
        f["auth"] = Value(exp->auth_type);
        f["root"] = Value(stringListToValue(exp->root));
        f["rw"] = Value(stringListToValue(exp->rw));
        f["ro"] = Value(stringListToValue(exp->ro));
        f["anonuid"] = Value(exp->anonuid);
        f["anongid"] = Value(exp->anongid);
        f["options"] = Value(exp->options);
        return Value(f);
    }
    return Value();

}

lsmStorageCapabilities *valueToCapabilities(Value &exp)
{
    lsmStorageCapabilities *rc = NULL;
    if( isExpectedObject(exp, "Capabilities") ) {
        const char *val = exp["cap"].asC_str();
        rc = lsmCapabilityRecordAlloc(val);
    }
    return rc;
}

Value capabilitiesToValue(lsmStorageCapabilities *cap)
{
    if( LSM_IS_CAPABILITIY(cap) ) {
        std::map<std::string, Value> c;
        char *t = capabilityString(cap);
        c["class"] = Value("Capabilities");
        c["cap"] = Value(t);
        free(t);
        return Value(c);
    }
    return Value();
}
