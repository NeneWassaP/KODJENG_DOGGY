// Import the functions you need from the SDKs you need
import { initializeApp } from "https://www.gstatic.com/firebasejs/9.17.1/firebase-app.js";
import {
  getDatabase,
  ref,
  set,
  push,
  get,
  remove,
  child,
  query,
  limitToLast,
  onValue,
} from "https://www.gstatic.com/firebasejs/9.17.1/firebase-database.js";



// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyA8CHNN1ipl17EQAPyoTb0WVMAWe2PRrAg",
  authDomain: "kodjengdoggy.firebaseapp.com",
  databaseURL:
    "https://kodjengdoggy-default-rtdb.asia-southeast1.firebasedatabase.app/",
  projectId: "kodjengdoggy",
  storageBucket: "kodjengdoggy.firebasestorage.app",
  messagingSenderId: "817037899250",
  appId: "1:817037899250:web:53aa8546f2f33237e6276b",
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

function addData() {
  const time = document.getElementById("alarm-time");
  const gram = document.getElementById("gram");

    // Reference to 'Alarm' with push to generate a unique key
    if (time.value!='' && gram.value!='' && gram.value>0){
      console.log(time)
      const dbRef = push(ref(db, 'Alarm_Default'));

      set(dbRef, {
          Time: time.value,
          Gram: gram.value,
          status: true
      }).then(() => {
          console.log("Data Added");
      }).catch((error) => {
          alert("Boommmmm");
          console.log(error);
      });

      

      time.value = '';
      gram.value = '';
      window.location.reload(true);
    }
};

function addDataCus(){
  const time_cus = document.getElementById('alarm-time-cus');
  const gram_cus = document.getElementById('gram-cus');
  const date = document.getElementById('date');
  const now = new Date(); // Get the current date
  const inputDate = new Date(date.value)
    
    // Set time of both dates to midnight for date-only comparison
  inputDate.setHours(0,0,0,0)
  now.setHours(0,0,0,0)
  if (time_cus.value!='' && gram_cus.value!='' && gram_cus.value>0 && date.value!='' && inputDate>=now ){
    const dbRef_cus = push(ref(db, 'Alarm_Customize'));

    set(dbRef_cus, {
      Date: date.value,
      Time: time_cus.value,
      Gram: gram_cus.value,
      status: true
    }).then(() => {
        console.log("Data Added");
    }).catch((error) => {
        alert("Boommmmm");
        console.log(error);
    });

    time_cus.value = '';
    gram_cus.value = '';
    date.value=''
    window.location.reload(true)
  }
}
function Notification() {
  const time = "";
  const gram = "";
  const now = new Date();
  const formattedDate = `${now.getDate().toString().padStart(2, "0")} ${now
    .toLocaleString("en-US", { month: "short" })
    .toUpperCase()} ${now.getFullYear()}`;
  const isEmpty = "";
  const dbNoti = push(ref(db, "Log"));

  set(dbNoti, {
    Date: formattedDate,
    Time: now.toLocaleTimeString(),
    Gram: 100,
    isEmpty: false,
    isEat: false
  })
    .then(() => {
      console.log("Data Added");
    })
    .catch((error) => {
      alert("Boommmmm");
      console.log(error);
    });

  window.location.reload(true);
}

export async function getAllData() {
  const dbRef = ref(getDatabase());
  var data;
  await get(child(dbRef, `Alarm_Default`))
    .then((snapshot) => {
      if (snapshot.exists()) {
        // console.log(snapshot.val());
        data = snapshot.val();
      } else {
        console.log("No data available");
      }
    })
    .catch((error) => {
      console.error(error);
    });
  if (data) {
    return data;
  } else {
    console.log("BOOOMMMM");
  }
}

export async function getAllDataCus() {
  const dbRef = ref(getDatabase());
  var data;
  await get(child(dbRef, `Alarm_Customize`))
    .then((snapshot) => {
      if (snapshot.exists()) {
        // console.log(snapshot.val());
        data = snapshot.val();
      } else {
        console.log("No data available");
      }
    })
    .catch((error) => {
      console.error(error);
    });
  if (data) {
    return data;
  } else {
    console.log("BOOOMMMM");
  }
}
export async function getAllNoti() {
  const dbRef = ref(getDatabase());
  var data;
  const logQuery = query(child(dbRef, "Log"), limitToLast(5)); // Limit to first 5 entries
  await get(logQuery)
    .then((snapshot) => {
      if (snapshot.exists()) {
        // console.log(snapshot.val());
        data = Object.values(snapshot.val()).reverse();
      } else {
        console.log("No data available");
      }
    })
    .catch((error) => {
      console.error(error);
    });
  if (data) {
    return data;
  } else {
    console.log("BOOOMMMM");
  }
}

export async function checkFoodTank() {
  const dbRef = ref(getDatabase());
  var data;
  const logQuery = query(child(dbRef, "Log"), limitToLast(1)); // Limit to last
  await get(logQuery)
    .then((snapshot) => {
      if (snapshot.exists()) {

        console.log(snapshot.val());
        data = Object.values(snapshot.val()).reverse();
        console.log(data);
      } else {
        console.log("No data available");
      }
    })
    .catch((error) => {
      console.error(error);
    });
  if (data) {
    return data;
  } else {
    console.log("BOOOMMMM");
  }
}

export async function editAlarm(uniqueID) {
  var snapshot = await get(ref(db, `Alarm_Default/${uniqueID}`));
  if (snapshot.exists()) {
    const data = snapshot.val();
    const status = data.status;
    set(ref(db, `Alarm_Default/${uniqueID}/status`), !status);
    console.log("Change Status ream roy");
  } else {
    console.log("No good la na!");
  }
}

export async function editAlarmCus(uniqueID) {
  var snapshot = await get(ref(db, `Alarm_Customize/${uniqueID}`));
  if (snapshot.exists()) {
    const data = snapshot.val();
    const status = data.status;
    set(ref(db, `Alarm_Customize/${uniqueID}/status`), !status);
    console.log("Change Status ream roy");
  } else {
    console.log("No good la na!");
  }
}

export async function deleteAlarm(id) {
  try {
    // Reference to the specific alarm node by unique ID
    const alarmRef = ref(db, `Alarm_Default/${id}`);

    await remove(alarmRef);
    window.location.reload(true);
    console.log(`Alarm with ID ${id} deleted successfully.`);
  } catch (error) {
    console.error("Error deleting alarm:", error);
  }
}

export async function deleteAlarmCus(id) {
  try {
    // Reference to the specific alarm node by unique ID
    const alarmRef = ref(db, `Alarm_Customize/${id}`);

    await remove(alarmRef);
    window.location.reload(true);
    console.log(`Alarm with ID ${id} deleted successfully.`);
  } catch (error) {
    console.error("Error deleting alarm:", error);
  }
}

const addBtn = document.getElementById("addBtn");
addBtn.addEventListener("click", addData);
const addBtnCus = document.getElementById("addBtnCus");
addBtnCus.addEventListener("click", addDataCus);