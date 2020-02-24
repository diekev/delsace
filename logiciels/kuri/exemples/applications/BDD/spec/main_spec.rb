describe 'database' do
    def run_script(commands)
        raw_output = nil
        IO.popen("./a.out", "r+") do |pipe|
            commands.each do |command|
                begin
                    pipe.puts command
                rescue Errno::EPIPE
                    break
                end
            end

            pipe.close_write

            raw_output = pipe.gets(nil)
        end

        raw_output.split("\n")
    end

    it 'inserts and retrieves a row' do
        result = run_script([
            "insère 1 user1 person1@example.com",
            "sélectionne",
            ".sors"
        ])

        expect(result).to match_array([
            "db > Exécution réussie",
            "db > (1, user1, person1@example.com)",
            "Exécution réussie",
            "db > ",
        ])
    end
end