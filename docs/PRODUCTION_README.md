# Using StoneDB-engine as Your Primary Database

## Current Limitations

StoneDB-engine **works** for basic use cases, but has these limitations for production use:

### Recently Added:
1. **Database path selection** - Use `--db <path>` to specify database file
2. **Batch mode** - Use `--batch` for non-interactive script execution  
3. **Backup/restore** - Use `backup <path>` and `restore <path>` commands

### Still Missing for Production:
1. **Single-file limitation** - All data in one file, limited scalability
2. **No query language** - Only key-value operations (no SQL, no filtering)
3. **No concurrent connections** - Multiple CLI instances can corrupt data
4. **No configuration** - Can't tune cache size, page size, etc.
5. **No authentication** - Anyone can access the database
6. **Limited error recovery** - Few safeguards against corruption

## What Works Now:

```bash
# Basic operations
./build/stone
stonedb> put user:1001 '{"name":"John","email":"john@example.com"}'
stonedb> get user:1001
stonedb> scan
stonedb> del user:1001
```

**Data persists** - Your data survives restarts:
```bash
# First session
./build/stone
stonedb> put app:config '{"setting":"value"}'
stonedb> quit

# Second session - data is still there
./build/stone
stonedb> get app:config
'{"setting":"value"}'
```

## How to Make It Production-Ready

### Completed Features:

1. **Command-line argument for database path:**
   ```bash
   ./build/stone --db /var/lib/mydb/app.sdb
   ./build/stone -d ./myapp.sdb
   ```

2. **Batch mode for scripts:**
   ```bash
   echo "put key value" | ./build/stone --batch
   ./build/stone --batch < commands.txt
   ```

3. **Backup and restore:**
   ```bash
   ./build/stone --db myapp.sdb
   stonedb> backup myapp_backup.json
   stonedb> restore myapp_backup.json
   ```

### Next Steps:

4. **Add init script for systemd:**
   ```systemd
   [Service]
   ExecStart=/usr/local/bin/stone --db /var/lib/mydb/data.sdb
   ```

### Medium Effort (2-3 weeks):

4. **Multiple database support:**
   ```bash
   ./build/stone --db production.sdb
   ./build/stone --db staging.sdb
   ```

5. **Simple query language:**
   ```bash
   stonedb> query user:* --filter "age > 25"
   stonedb> count user:*
   ```

6. **Export/Import:**
   ```bash
   ./build/stone --export data.json
   ./build/stone --import backup.json
   ```

### Major Features (Months of work):

7. **Server mode** (like MySQL daemon):
   ```bash
   stonedb-server --port 3306 --db /var/lib/mydb
   # Then connect from multiple clients
   ```

8. **SQL-like queries**
9. **Indexes for fast lookups**
10. **Replication and clustering**

## Current Real-World Usage Pattern

**Best for:**
- Personal projects
- Learning/educational use
- Small apps with <1000 records
- Single-user applications

**Not recommended for:**
- Multi-user production systems
- High-traffic applications
- Applications requiring complex queries
- Critical business data (without backups)

## Making It Your Own

To customize for your needs:

1. **Change default database location:**
   Edit `src/main.cpp` line 29:
   ```cpp
   if(!storage->open("/var/lib/myapp/mydb.sdb"))
   ```

2. **Add startup script:**
   ```bash
   #!/bin/bash
   # /usr/local/bin/stonedb
   cd /opt/StoneDB-engine/build
   ./stone
   ```

3. **Create systemd service:**
   ```ini
   [Unit]
   Description=StoneDB Database
   After=network.target
   
   [Service]
   Type=simple
   ExecStart=/opt/StoneDB-engine/build/stone
   Restart=always
   
   [Install]
   WantedBy=multi-user.target
   ```

## Testing Real-World Usage

Test your database with realistic workloads:

```bash
# Load test data
for i in {1..100}; do
  ./build/stone <<EOF
put user:$i '{"id":$i,"name":"User$i","active":true}'
EOF
done

# Query patterns
./build/stone <<EOF
scan | grep "user:"
get user:42
put user:42 '{"id":42,"name":"UpdatedUser","active":true}'
EOF
```

## Recommendation

**Current state:** Good for learning, single-user apps, small projects
**For production:** Add database path selection and batch mode first, then consider server mode

The core ACID guarantees work - you just need better tooling around it!

