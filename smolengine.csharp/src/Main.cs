using System;
using SmolEngine;
using System.Timers;

namespace SmolEngine
{
    public class Tests
    {
        Tests()
        {
            Console.WriteLine("Test init");
        }

        private void CallMe(uint val1, uint val2)
        {
            Console.WriteLine($"Value1: {val1}, Value2: {val2}");
        }
    }
}

public class PlayerContoller: BehaviourPrimitive
{
    RigidBodyComponent rb;

    private void OnBegin()
    {
        GetComponent<RigidBodyComponent>(ref rb);
    }

    private void OnUpdate()
    {
        if (Input.IsKeyPressed(KeyCode.W))
        {
            rb.AddImpulse(new Vector3(1, 0, 0));
            rb.AddTorque(new Vector3(0, 0, -5));
        }

        if (Input.IsKeyPressed(KeyCode.S))
        {
            rb.AddImpulse(new Vector3(-1, 0, 0));
            rb.AddTorque(new Vector3(0, 0, 5));
        }

        if (Input.IsKeyPressed(KeyCode.D))
        {
            rb.AddTorque(new Vector3(2, 0, 0));
            rb.AddImpulse(new Vector3(0, 0, 1));
        }

        if (Input.IsKeyPressed(KeyCode.A))
        {
            rb.AddTorque(new Vector3(-2, 0, 0));
            rb.AddImpulse(new Vector3(0, 0, -1));

        }

        if (Input.IsKeyPressed(KeyCode.Space))
            rb.AddImpulse(new Vector3(0, 10, 0));
    }

    private void OnDestroy()
    {

    }
}

public class BallSpawner: BehaviourPrimitive
{
    private TransformComponent spawn_point  = new TransformComponent();
    public Prefab BallPref = null;
    public Prefab BallPref2 = null;
    public Prefab BallPref3 = null;

    public float  Speed = 0.0f;
    public float  Health = 0.0f;
    public string Tag = "";

    private Timer Timer;

    private void OnBegin()
    {
       GetComponent<TransformComponent>(ref spawn_point);


        Timer = new Timer(2000.0f);
        // Hook up the Elapsed event for the timer. 
        Timer.Elapsed += OnTimedEvent;
        Timer.AutoReset = true;
        Timer.Enabled = true;


        Timer.Start();

        SLog.WriteLine("Begin");
    }

    private void OnTimedEvent(Object source, ElapsedEventArgs e)
    {
        SpawnBall();
    }

    void SpawnBall()
    {
        Prefab pref = null;
        Random random = new System.Random();
        int num = random.Next(0, 3);

        switch(num)
        {
            case 0:
                {
                    pref = BallPref;
                    break;
                }
            case 1:
                {
                    pref = BallPref2;
                    break;
                }

            case 2:
                {
                    pref = BallPref3;
                    break;
                }
        }

        Actor ball = pref.Instantiate(new Vector3(GetRandomNumber(spawn_point.Position.X, spawn_point.Position.X + 50),
spawn_point.Position.Y, GetRandomNumber(spawn_point.Position.Z, spawn_point.Position.Z + 50)));

        RigidBodyComponent rb = new RigidBodyComponent();
        if (ball.GetComponent<RigidBodyComponent>(ref rb))
        {
            rb.AddImpulse(new Vector3(0, -40.0f, 0));
        }
    }

    private void OnUpdate() 
    {

    }

    private void OnDestroy() { }

    public float GetRandomNumber(double minimum, double maximum)
    {
        Random random = new Random();
        double res = random.NextDouble() * (maximum - minimum) + minimum;
        return (float)res;
    }
}



