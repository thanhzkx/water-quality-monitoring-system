import express from "express";
import fetch from "node-fetch";
import cors from "cors";
import mysql from "mysql2/promise";
import bcrypt from "bcryptjs";
import jwt from "jsonwebtoken";

const app = express();

const db = mysql.createPool({
  host: "mysql",
  user: "wateruser",
  password: "***",
  database: "water_monitor",
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
});

app.use(cors({
  origin: [
    "http://localhost:5173",
    "http://180.93.228.86",
    "http://180.93.228.86:1880"
  ],
  methods: ["GET", "POST"],
  allowedHeaders: ["Content-Type", "Authorization"]
}));

app.use(express.json());


// ==================== JWT AUTH ====================

function authenticateToken(req, res, next) {

  const authHeader = req.headers["authorization"];
  const token = authHeader && authHeader.split(" ")[1];

  if (!token) {
    return res.status(401).json({
      success: false,
      message: "Chưa đăng nhập"
    });
  }

  jwt.verify(
    token,
    process.env.JWT_SECRET,
    (err, user) => {

      if (err) {
        return res.status(403).json({
          success: false,
          message: "Token không hợp lệ hoặc đã hết hạn"
        });
      }

      req.user = user;
      next();
    }
  );
}


// ==================== ROOT ====================

app.get("/", (req, res) => {
  res.send("OK");
});


// ==================== DEVICES ====================

app.get("/api/devices", authenticateToken, async (req, res) => {

  try {

    const sql = `
      SELECT
        device_id,
        machine_name,
        status,
        last_seen
      FROM devices
      ORDER BY machine_name ASC
    `;

    const [rows] = await db.execute(sql);

    res.json({
      success: true,
      devices: rows
    });

  } catch (err) {

    console.error("Get devices error:", err);

    res.status(500).json({
      success: false,
      error: "Không thể lấy danh sách thiết bị"
    });
  }
});


// ==================== CHATBOT ====================

app.options("/chat", cors());

const API_KEY = process.env.GROQ_API_KEY;

app.post("/chat", async (req, res) => {

  try {

    const { messages } = req.body;

    const response = await fetch(
      "https://api.groq.com/openai/v1/chat/completions",
      {
        method: "POST",

        headers: {
          "Content-Type": "application/json",
          "Authorization": `Bearer ${API_KEY}`
        },

        body: JSON.stringify({
          model: "openai/gpt-oss-20b",
          messages
        })
      }
    );

    const data = await response.json();

    console.log(
      "📤 AI RAW:",
      JSON.stringify(data, null, 2)
    );

    console.log("📤 CHAT:", data);

    res.json(data);

  } catch (err) {

    console.error("❌ CHAT ERROR:", err);

    res.status(500).json({
      error: "Chat lỗi"
    });
  }
});


// ==================== ANALYZE WATER ====================

app.post("/analyze-water", async (req, res) => {

  try {

    const {
      temperature,
      ph,
      turbidity
    } = req.body;

    console.log("📥 DATA:", req.body);

    const issues = [];

    if (ph < 6.5 || ph > 8.5) {
      issues.push(`pH bất thường (${ph})`);
    }

    if (turbidity > 5) {
      issues.push(`Độ đục cao (${turbidity} NTU)`);
    }

    if (temperature < 20 || temperature > 30) {
      issues.push(`Nhiệt độ bất thường (${temperature}°C)`);
    }

    if (issues.length === 0) {

      return res.json({
        message: "✅ Nước đạt tiêu chuẩn an toàn."
      });
    }

    const prompt = `
Dữ liệu nước:

- Nhiệt độ: ${temperature}°C
- pH: ${ph}
- Độ đục: ${turbidity} NTU

Vấn đề: ${issues.join(", ")}

Hãy:

- Giải thích tình trạng
- Nguyên nhân
- Cách xử lý

Trả lời ngắn gọn bằng tiếng Việt.
`;

    const response = await fetch(
      "https://api.groq.com/openai/v1/chat/completions",
      {
        method: "POST",

        headers: {
          "Content-Type": "application/json",
          "Authorization": `Bearer ${API_KEY}`
        },

        body: JSON.stringify({
          model: "openai/gpt-oss-20b",
          messages: [
            {
              role: "user",
              content: prompt
            }
          ]
        })
      }
    );

    const data = await response.json();

    console.log(
      "📤 AI RAW:",
      JSON.stringify(data, null, 2)
    );

    const result =
      data?.choices?.[0]?.message?.content ||
      "Không phân tích được";

    res.json({
      warning: true,
      issues,
      analysis: result
    });

  } catch (err) {

    console.error("❌ ERROR:", err);

    res.status(500).json({
      error: "Server lỗi"
    });
  }
});


// ==================== SENSOR HISTORY ====================

app.get(
  "/sensor-history",
  authenticateToken,
  async (req, res) => {

    try {

      const sql = `
        SELECT sensor_data.*
        FROM sensor_data
        ORDER BY sensor_data.created_at DESC
      `;

      const [rows] = await db.execute(sql);

      res.json(rows);

    } catch (err) {

      console.error(err);

      res.status(500).json({
        error: "SQL lỗi"
      });
    }
  }
);


// ==================== AI ANALYSIS ====================

app.post(
  "/ai-analysis",
  authenticateToken,
  async (req, res) => {

    try {

      const {
        question,
        device_id
      } = req.body;

      if (!question) {
        return res.status(400).json({
          error: "Thiếu câu hỏi"
        });
      }

      let sql = `
        SELECT
          device_id,
          temperature,
          ph,
          turbidity,
          temp_status,
          ph_status,
          tur_status,
          created_at
        FROM sensor_data
        WHERE 1 = 1
      `;

      const params = [];


      // =========================
      // 1. LỌC DEVICE
      // =========================

      if (device_id) {

        sql += ` AND device_id = ? `;

        params.push(device_id);
      }


      // =========================
      // 2. PHÂN TÍCH NGÀY CỤ THỂ
      // =========================

      const dateMatch = question.match(
        /(\d{1,2})\/(\d{1,2})(?:\/(\d{4}))?/
      );

      if (dateMatch) {

        const day = dateMatch[1];
        const month = dateMatch[2];
        const year =
          dateMatch[3] ||
          new Date().getFullYear();

        const dateString =
          `${year}-${String(month).padStart(2, "0")}-${String(day).padStart(2, "0")}`;

        sql += `
          AND created_at >= ?
          AND created_at < DATE_ADD(?, INTERVAL 1 DAY)
        `;

        params.push(
          dateString,
          dateString
        );

      }

      // =========================
      // 3. HÔM NAY
      // =========================

      else if (
        question
          .toLowerCase()
          .includes("hôm nay")
      ) {

        sql += `
          AND DATE(created_at) = CURDATE()
        `;
      }


      // =========================
      // 4. HÔM QUA
      // =========================

      else if (
        question
          .toLowerCase()
          .includes("hôm qua")
      ) {

        sql += `
          AND DATE(created_at)
              = CURDATE() - INTERVAL 1 DAY
        `;
      }


      // =========================
      // 5. 7 NGÀY / TUẦN
      // =========================

      else if (
        question.toLowerCase().includes("7 ngày") ||
        question.toLowerCase().includes("tuần")
      ) {

        sql += `
          AND created_at >= NOW() - INTERVAL 7 DAY
        `;
      }


      // =========================
      // 6. THÁNG
      // =========================

      else if (
        question
          .toLowerCase()
          .includes("tháng")
      ) {

        sql += `
          AND created_at >= NOW() - INTERVAL 30 DAY
        `;
      }


      // =========================
      // 7. SẮP XẾP
      // =========================

      sql += `
        ORDER BY created_at ASC
      `;


      // =========================
      // 8. LẤY DỮ LIỆU
      // =========================

      const [rows] =
        await db.execute(sql, params);

      console.log(
        `📊 AI Analysis: ${rows.length} rows`
      );


      // =========================
      // 9. KHÔNG CÓ DỮ LIỆU
      // =========================

      if (rows.length === 0) {

        return res.json({
          sql_used: sql,
          total_rows: 0,
          analysis:
            "Không tìm thấy dữ liệu phù hợp với yêu cầu."
        });
      }


      // =========================
      // 10. THỐNG KÊ
      // =========================

      const numericStats = (values) => {

        const nums = values
          .map(Number)
          .filter(v => Number.isFinite(v));

        if (nums.length === 0) {

          return {
            valid_count: 0,
            null_count: 0,
            min: null,
            max: null,
            average: null,
            range: null
          };
        }

        const min = Math.min(...nums);
        const max = Math.max(...nums);

        const average =
          nums.reduce(
            (sum, value) =>
              sum + value,
            0
          ) / nums.length;

        return {

          valid_count: nums.length,

          min:
            Number(min.toFixed(3)),

          max:
            Number(max.toFixed(3)),

          average:
            Number(average.toFixed(3)),

          range:
            Number(
              (max - min).toFixed(3)
            )
        };
      };


      // =========================
      // THỐNG KÊ THEO DEVICE
      // =========================

      const deviceStats = {};

      for (const row of rows) {

        const id =
          row.device_id ||
          "UNKNOWN";

        if (!deviceStats[id]) {

          deviceStats[id] = {

            total_records: 0,

            temperature: [],
            ph: [],
            turbidity: [],

            temperature_null: 0,
            ph_null: 0,
            turbidity_null: 0,

            status: {
              temperature: {},
              ph: {},
              turbidity: {}
            }
          };
        }

        deviceStats[id].total_records++;


        // Temperature

        if (
          row.temperature !== null &&
          row.temperature !== undefined &&
          Number.isFinite(
            Number(row.temperature)
          )
        ) {

          deviceStats[id]
            .temperature
            .push(
              Number(row.temperature)
            );

        } else {

          deviceStats[id]
            .temperature_null++;
        }


        // pH

        if (
          row.ph !== null &&
          row.ph !== undefined &&
          Number.isFinite(
            Number(row.ph)
          )
        ) {

          deviceStats[id]
            .ph
            .push(
              Number(row.ph)
            );

        } else {

          deviceStats[id]
            .ph_null++;
        }


        // Turbidity

        if (
          row.turbidity !== null &&
          row.turbidity !== undefined &&
          Number.isFinite(
            Number(row.turbidity)
          )
        ) {

          deviceStats[id]
            .turbidity
            .push(
              Number(row.turbidity)
            );

        } else {

          deviceStats[id]
            .turbidity_null++;
        }


        // Sensor status

        if (row.temp_status) {

          deviceStats[id]
            .status
            .temperature[
              row.temp_status
            ] =
            (
              deviceStats[id]
                .status
                .temperature[
                  row.temp_status
                ] || 0
            ) + 1;
        }

        if (row.ph_status) {

          deviceStats[id]
            .status
            .ph[
              row.ph_status
            ] =
            (
              deviceStats[id]
                .status
                .ph[
                  row.ph_status
                ] || 0
            ) + 1;
        }

        if (row.tur_status) {

          deviceStats[id]
            .status
            .turbidity[
              row.tur_status
            ] =
            (
              deviceStats[id]
                .status
                .turbidity[
                  row.tur_status
                ] || 0
            ) + 1;
        }
      }


      // =========================
      // SUMMARY
      // =========================

      const summary = {};

      for (
        const [id, data]
        of Object.entries(deviceStats)
      ) {

        summary[id] = {

          total_records:
            data.total_records,

          temperature: {
            ...numericStats(
              data.temperature
            ),
            null_count:
              data.temperature_null
          },

          ph: {
            ...numericStats(data.ph),
            null_count:
              data.ph_null
          },

          turbidity: {
            ...numericStats(
              data.turbidity
            ),
            null_count:
              data.turbidity_null
          },

          status:
            data.status
        };
      }


      console.log(
        "📊 AI SUMMARY:",
        JSON.stringify(
          summary,
          null,
          2
        )
      );


      // =========================
      // PROMPT AI
      // =========================

      const prompt = `

Bạn là AI phân tích dữ liệu cho hệ thống giám sát chất lượng nước.

Người dùng hỏi:

${question}

${
  device_id
    ? `Thiết bị được yêu cầu:
${device_id}`
    : "Người dùng chưa chỉ định thiết bị cụ thể."
}

Dữ liệu đã được hệ thống thống kê từ database MySQL.

TỔNG SỐ BẢN GHI:

${rows.length}

THỐNG KÊ THEO THIẾT BỊ:

${JSON.stringify(
  summary,
  null,
  2
)}

YÊU CẦU:

1. Chỉ sử dụng thông tin được cung cấp trong dữ liệu thống kê.

2. Không được tự tạo hoặc suy đoán:
- ngưỡng an toàn
- tiêu chuẩn nước
- giới hạn kỹ thuật
- nguyên nhân vật lý hoặc hóa học

3. Không được kết luận nước "an toàn",
"không an toàn", "đạt tiêu chuẩn" hoặc
"không đạt tiêu chuẩn" nếu không có tiêu chuẩn
được cung cấp để đối chiếu.

4. Hãy phân tích:
- tổng số bản ghi
- số lượng mẫu của từng device
- nhiệt độ: min, max, trung bình, khoảng dao động
- pH: min, max, trung bình, khoảng dao động
- turbidity: min, max, trung bình, khoảng dao động
- trạng thái sensor nếu có
- sự khác biệt giữa các device nếu có
- xu hướng hoặc mức dao động dựa trên các thống kê được cung cấp

5. Khi phát hiện giá trị bất thường:
- chỉ mô tả giá trị bất thường
- không tự khẳng định nguyên nhân
- không tự áp dụng tiêu chuẩn bên ngoài

6. Nếu có nhiều device_id,
phải phân tích riêng từng device,
không gộp dữ liệu của các thiết bị thành một thiết bị.

7. Nếu dữ liệu không đủ để kết luận một vấn đề,
hãy nói rõ rằng dữ liệu hiện tại chưa đủ.

8. Trả lời bằng tiếng Việt.

Cấu trúc:

### 1. Tổng quan dữ liệu

### 2. Nhiệt độ

### 3. pH

### 4. Turbidity

### 5. Trạng thái và bất thường

### 6. Kết luận

`;


      // =========================
      // GỌI GROQ
      // =========================

      const response = await fetch(
        "https://api.groq.com/openai/v1/chat/completions",
        {
          method: "POST",

          headers: {
            "Content-Type": "application/json",
            "Authorization":
              `Bearer ${API_KEY}`
          },

          body: JSON.stringify({

            model:
              "openai/gpt-oss-20b",

            messages: [
              {
                role: "user",
                content: prompt
              }
            ]
          })
        }
      );

      const data =
        await response.json();


      console.log(
        "📤 AI ANALYSIS RAW:",
        JSON.stringify(
          data,
          null,
          2
        )
      );


      // =========================
      // TRẢ KẾT QUẢ
      // =========================

      res.json({

        sql_used: sql,

        total_rows:
          rows.length,

        device_id:
          device_id || null,

        analysis:
          data?.choices?.[0]
            ?.message?.content ||
          "Không phân tích được dữ liệu."
      });

    } catch (err) {

      console.error(
        "❌ AI ANALYSIS ERROR:",
        err
      );

      res.status(500).json({
        error: "AI lỗi"
      });
    }
  }
);


// ==================== LOGIN ====================

app.post("/api/login", async (req, res) => {

  try {

    const {
      username,
      password
    } = req.body;

    if (!username || !password) {

      return res.status(400).json({
        success: false,
        message:
          "Thiếu username hoặc password"
      });
    }


    const [rows] =
      await db.execute(
        `
        SELECT
          id,
          username,
          password_hash,
          role
        FROM users
        WHERE username = ?
        `,
        [username]
      );


    if (rows.length === 0) {

      return res.status(401).json({
        success: false,
        message:
          "Sai username hoặc password"
      });
    }


    const user = rows[0];


    const passwordValid =
      await bcrypt.compare(
        password,
        user.password_hash
      );


    if (!passwordValid) {

      return res.status(401).json({
        success: false,
        message:
          "Sai username hoặc password"
      });
    }


    const token =
      jwt.sign(
        {
          user_id: user.id,
          username: user.username,
          role: user.role
        },
        process.env.JWT_SECRET,
        {
          expiresIn: "8h"
        }
      );


    res.json({

      success: true,

      token,

      user: {
        id: user.id,
        username: user.username,
        role: user.role
      }
    });


  } catch (err) {

    console.error(
      "❌ LOGIN ERROR:",
      err
    );

    res.status(500).json({
      success: false,
      message: "Lỗi server"
    });
  }
});


// ==================== UPDATE MACHINE NAME ====================

app.post(
  "/api/devices/name",
  authenticateToken,
  async (req, res) => {

    try {

      const {
        device_id,
        machine_name
      } = req.body;


      if (!device_id || !machine_name) {

        return res.status(400).json({
          success: false,
          message:
            "device_id and machine_name are required"
        });
      }


      if (
        !/^[0-9A-Fa-f]{12}$/.test(
          device_id
        )
      ) {

        return res.status(400).json({
          success: false,
          message:
            "Invalid device_id"
        });
      }


      if (
        typeof machine_name !== "string" ||
        machine_name.length < 1 ||
        machine_name.length > 50
      ) {

        return res.status(400).json({
          success: false,
          message:
            "Invalid machine_name"
        });
      }


      const [result] =
        await db.execute(
          `
          UPDATE devices
          SET machine_name = ?
          WHERE device_id = ?
          `,
          [
            machine_name,
            device_id
          ]
        );


      if (result.affectedRows === 0) {

        return res.status(404).json({
          success: false,
          message:
            "Không tìm thấy thiết bị"
        });
      }


      res.json({
        success: true,
        message:
          "Machine name updated successfully"
      });


    } catch (err) {

      console.error(
        "Update machine name error:",
        err
      );

      res.status(500).json({
        success: false,
        message:
          "Lỗi server"
      });
    }
  }
);


// ==================== START SERVER ====================

app.listen(
  3000,
  () =>
    console.log(
      "Server running on port 3000"
    )
);
