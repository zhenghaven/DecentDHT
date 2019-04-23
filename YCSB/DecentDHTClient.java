package com.yahoo.ycsb.db;
import com.yahoo.ycsb.ByteIterator;
import com.yahoo.ycsb.DB;
import com.yahoo.ycsb.DBException;
import com.yahoo.ycsb.Status;
import com.yahoo.ycsb.StringByteIterator;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;
import java.util.Set;
import java.util.Vector;

import org.json.JSONObject;

/**
 * YCSB binding for DecentDHT.
 */
public class DecentDHTClient extends DB {

  /**
   * Private memebers such as DHT connections
   * or other handlers which can be used to contact DHT when reading and writting data.
   */

  private boolean isInited = false;

  public void init() throws DBException {

    // Initialzie the DHT here.

    isInited = true;
  }

  public void cleanup() throws DBException {
    if (isInited) {
    
    // Deconstruct objects initilized in the init() function.
      isInited = false;
    }
  }

  @Override
  public Status read(String table, String key, Set<String> fields, Map<String, ByteIterator> result) {
    byte[] buffer;
    
    // table is ignored in this function.
    // Given the key, retrieve the value from DHT
    // Convert the data retrieved to Json doc.

    try {
     
      // Retrieve the buffer by the data retrived. Example: readResult.getBuffer().get(buffer);
    } catch (RadosException e) {
      return new Status("ERROR-" + e.getReturnValue(), e.getMessage());
    }

    // The rest of the code should be similar in our implementation. 
    // I think we can just use the codes here without changes. 
    JSONObject json = new JSONObject(new String(buffer, java.nio.charset.StandardCharsets.UTF_8));
    Set<String> fieldsToReturn = (fields == null ? json.keySet() : fields);

    for (String name : fieldsToReturn) {
      result.put(name, new StringByteIterator(json.getString(name)));
    }

    return result.isEmpty() ? Status.ERROR : Status.OK;
  }


  @Override
  public Status insert(String table, String key, Map<String, ByteIterator> values) {

    // table is ignored again.
    // No need to change the codes below.
    JSONObject json = new JSONObject();
    for (final Entry<String, ByteIterator> e : values.entrySet()) {
      json.put(e.getKey(), e.getValue().toString());
    }

    try {
      // Write the key and the value to our DHT here. Example: ioctx.write(key, json.toString());
    } catch (RadosException e) {
      return new Status("ERROR-" + e.getReturnValue(), e.getMessage());
    }
    return Status.OK;
  }

  @Override
  public Status delete(String table, String key) {

    try {
      // very simple function, just remove the data associated with the key given. table is not used.
      // Example: ioctx.remove(key);
    } catch (RadosException e) {
      return new Status("ERROR-" + e.getReturnValue(), e.getMessage());
    }
    return Status.OK;
  }

  @Override
  public Status update(String table, String key, Map<String, ByteIterator> values) {
    // Just call delete() function first and then call insert() function.
    // No need to change the code.
    Status rtn = delete(table, key);
    if (rtn.equals(Status.OK)) {
      return insert(table, key, values);
    }
    return rtn;
  }

  @Override
  public Status scan(String table, String startkey, int recordcount, Set<String> fields,
      // I guess we don't need to implement this as well. lol.
      Vector<HashMap<String, ByteIterator>> result) {
    return Status.NOT_IMPLEMENTED;
  }
}
